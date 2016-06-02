#pragma once

/* Document Markup Language (DML) v1.0 parser
 * revision 0.03
 */

namespace nall { namespace {

struct DML {
  auto& setAllowHTML(bool allowHTML) { settings.allowHTML = allowHTML; return *this; }
  auto& setHost(const string& hostname) { settings.host = {hostname, "/"}; return *this; }
  auto& setPath(const string& pathname) { settings.path = pathname; return *this; }
  auto& setReader(const function<string (string)>& reader) { settings.reader = reader; return *this; }
  auto& setSectioned(bool sectioned) { settings.sectioned = sectioned; return *this; }

  auto parse(const string& filedata, const string& pathname) -> string;
  auto parse(const string& filename) -> string;

private:
  struct Settings {
    bool allowHTML = true;
    string host = "localhost/";
    string path;
    function<string (string)> reader;
    bool sectioned = true;
  } settings;

  struct State {
    string output;
    uint sections = 0;
  } state;

  auto parseDocument(const string& filedata, const string& pathname, uint depth) -> bool;
  auto parseBlock(string& block, const string& pathname, uint depth) -> bool;
  auto count(const string& text, char value) -> uint;

  auto escape(const string& text) -> string;
  auto markup(const string& text) -> string;
};

auto DML::parse(const string& filedata, const string& pathname) -> string {
  settings.path = pathname;
  parseDocument(filedata, settings.path, 0);
  return state.output;
}

auto DML::parse(const string& filename) -> string {
  if(!settings.path) settings.path = pathname(filename);
  string document = settings.reader ? settings.reader(filename) : string::read(filename);
  parseDocument(document, settings.path, 0);
  return state.output;
}

auto DML::parseDocument(const string& filedata, const string& pathname, uint depth) -> bool {
  if(depth >= 100) return false;  //attempt to prevent infinite recursion with reasonable limit

  auto blocks = filedata.split("\n\n");
  for(auto& block : blocks) parseBlock(block, pathname, depth);
  if(settings.sectioned && state.sections && depth == 0) state.output.append("</section>\n");
  return true;
}

auto DML::parseBlock(string& block, const string& pathname, uint depth) -> bool {
  if(!block.stripRight()) return true;
  auto lines = block.split("\n");

  //include
  if(block.beginsWith("<include ") && block.endsWith(">")) {
    string filename{pathname, block.trim("<include ", ">", 1L).strip()};
    string document = settings.reader ? settings.reader(filename) : string::read(filename);
    parseDocument(document, nall::pathname(filename), depth + 1);
  }

  //html
  else if(block.beginsWith("<html>\n") && settings.allowHTML) {
    for(auto n : range(lines)) {
      if(n == 0 || !lines[n].beginsWith("  ")) continue;
      state.output.append(lines[n].trimLeft("  ", 1L), "\n");
    }
  }

  //section
  else if(block.beginsWith("# ")) {
    if(settings.sectioned) {
      if(state.sections++) state.output.append("</section>");
      state.output.append("<section>");
    }
    auto content = lines.takeLeft().trimLeft("# ", 1L).split(" => ", 1L);
    auto data = markup(content[0]);
    auto name = escape(content(1, data.hash()));
    state.output.append("<header id=\"", name, "\">", data);
    for(auto& line : lines) {
      if(!line.beginsWith("# ")) continue;
      state.output.append("<span>", line.trimLeft("# ", 1L), "</span>");
    }
    state.output.append("</header>\n");
  }

  //header
  else if(auto depth = count(block, '=')) {
    auto content = slice(lines.takeLeft(), depth + 1).split(" => ", 1L);
    auto data = markup(content[0]);
    auto name = escape(content(1, data.hash()));
    if(depth <= 6) {
      state.output.append("<h", depth, " id=\"", name, "\">", data);
      for(auto& line : lines) {
        if(count(line, '=') != depth) continue;
        state.output.append("<span>", slice(line, depth + 1), "</span>");
      }
      state.output.append("</h", depth, ">\n");
    }
  }

  //navigation
  else if(count(block, '-')) {
    state.output.append("<nav>\n");
    uint level = 0;
    for(auto& line : lines) {
      if(auto depth = count(line, '-')) {
        while(level < depth) level++, state.output.append("<ul>\n");
        while(level > depth) level--, state.output.append("</ul>\n");
        auto content = slice(line, depth + 1).split(" => ", 1L);
        auto data = markup(content[0]);
        auto name = escape(content(1, data.hash()));
        state.output.append("<li><a href=\"#", name, "\">", data, "</a></li>\n");
      }
    }
    while(level--) state.output.append("</ul>\n");
    state.output.append("</nav>\n");
  }

  //list
  else if(count(block, '*')) {
    uint level = 0;
    for(auto& line : lines) {
      if(auto depth = count(line, '*')) {
        while(level < depth) level++, state.output.append("<ul>\n");
        while(level > depth) level--, state.output.append("</ul>\n");
        auto data = markup(slice(line, depth + 1));
        state.output.append("<li>", data, "</li>\n");
      }
    }
    while(level--) state.output.append("</ul>\n");
  }

  //quote
  else if(count(block, '>')) {
    uint level = 0;
    for(auto& line : lines) {
      if(auto depth = count(line, '>')) {
        while(level < depth) level++, state.output.append("<blockquote>\n");
        while(level > depth) level--, state.output.append("</blockquote>\n");
        auto data = markup(slice(line, depth + 1));
        state.output.append(data, "\n");
      }
    }
    while(level--) state.output.append("</blockquote>\n");
  }

  //code
  else if(block.beginsWith("  ")) {
    state.output.append("<pre>");
    for(auto& line : lines) {
      if(!line.beginsWith("  ")) continue;
      state.output.append(escape(line.trimLeft("  ", 1L)), "\n");
    }
    state.output.trimRight("\n", 1L).append("</pre>\n");
  }

  //divider
  else if(block.equals("---")) {
    state.output.append("<hr>\n");
  }

  //paragraph
  else {
    state.output.append("<p>", markup(block), "</p>\n");
  }

  return true;
}

auto DML::count(const string& text, char value) -> uint {
  for(uint n = 0; n < text.size(); n++) {
    if(text[n] != value) {
      if(text[n] == ' ') return n;
      break;
    }
  }
  return 0;
}

auto DML::escape(const string& text) -> string {
  string output;
  for(auto c : text) {
    if(c == '&') { output.append("&amp;"); continue; }
    if(c == '<') { output.append("&lt;"); continue; }
    if(c == '>') { output.append("&gt;"); continue; }
    if(c == '"') { output.append("&quot;"); continue; }
    output.append(c);
  }
  return output;
}

auto DML::markup(const string& text) -> string {
  string output;

  char match = 0;
  uint offset = 0;
  for(uint n = 0; n < text.size();) {
    char a = n ? text[n - 1] : 0;
    char b = text[n];
    char c = text[n++ + 1];

    bool d = !a || a == ' ' || a == '\t' || a == '\r' || a == '\n';  //is previous character whitespace?
    bool e = !c || c == ' ' || c == '\t' || c == '\r' || c == '\n';  //is next character whitespace?
    bool f = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');  //is next character alphanumeric?

    if(!match && d && !e) {
      if(b == '*') { match = '*'; offset = n; continue; }
      if(b == '/') { match = '/'; offset = n; continue; }
      if(b == '_') { match = '_'; offset = n; continue; }
      if(b == '~') { match = '~'; offset = n; continue; }
      if(b == '|') { match = '|'; offset = n; continue; }
      if(b == '[') { match = ']'; offset = n; continue; }
      if(b == '{') { match = '}'; offset = n; continue; }
    }

    //if we reach the end of the string without a match; force a match so the content is still output
    if(match && b != match && !c) { b = match; f = 0; n++; }

    if(match && b == match && !f) {
      match = 0;
      auto content = slice(text, offset, n - offset - 1);
      if(b == '*') { output.append("<strong>", escape(content), "</strong>"); continue; }
      if(b == '/') { output.append("<em>", escape(content), "</em>"); continue; }
      if(b == '_') { output.append("<ins>", escape(content), "</ins>"); continue; }
      if(b == '~') { output.append("<del>", escape(content), "</del>"); continue; }
      if(b == '|') { output.append("<code>", escape(content), "</code>"); continue; }
      if(b == ']') {
        auto p = content.split(" => ", 1L);
        p[0].replace("@/", settings.host);
        output.append("<a href=\"", escape(p[0]), "\">", escape(p(1, p[0])), "</a>");
        continue;
      }
      if(b == '}') {
        auto p = content.split(" => ", 1L);
        p[0].replace("@/", settings.host);
        output.append("<img src=\"", escape(p[0]), "\" alt=\"", escape(p(1, "")), "\">");
        continue;
      }
      continue;
    }

    if(match) continue;
    if(b == '\\' && c) { output.append(c); n++; continue; }  //character escaping
    if(b == '&') { output.append("&amp;"); continue; }       //entity escaping
    if(b == '<') { output.append("&lt;"); continue; }        //...
    if(b == '>') { output.append("&gt;"); continue; }        //...
    if(b == '"') { output.append("&quot;"); continue; }      //...
    output.append(b);
  }

  return output;
}

}}
