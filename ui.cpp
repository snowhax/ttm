
#include "ui.h"
#include <typeinfo>

static
void childXpath(COM::CCNode* node, std::vector<UI::Xpath*>& pool) {
    if (!node->_enabled) return;
    UI::Xpath* xpath = new UI::Xpath();
    xpath->instance = node;
    xpath->clazz = COM::clsName(node);
    xpath->name = node->_name;
    xpath->tag = node->_tag;
    xpath->enabled = node->_enabled;
    xpath->free = false;
    if (!node->_children.empty())
        for (COM::CCNode* child : node->_children)
            childXpath(child, xpath->children);
    pool.push_back(xpath);
}

UI::Xpath* UI::genXpath() {
    UI::Xpath* xpath = new UI::Xpath();
    xpath->enabled = false;
    xpath->name = "";
    xpath->clazz = "root";
    xpath->free = true;

    int display = r(COM::alsr(MemAddrDirector));
    if (!display)
        return xpath;

    COM::CCNode* root = (COM::CCNode*)r(display+0xe0);
    if (!root)
        return xpath;

    xpath->instance = root;
    xpath->enabled = true;

    childXpath(root, xpath->children);

    return xpath;
}

void UI::delXpath(UI::Xpath* root) {
    delete root;
}

static void _xpath2string(UI::Xpath* root, std::string& output, std::string prefix) {
  if (!root->children.size())
    return;

  for (UI::Xpath* child : root->children) {
    std::string str(prefix);
    str += ("tag:" + COM::toString(child->tag));
    str += (" name:" + child->name);
    str += (" class:" + child->clazz);
    output.append(str + "\n");
    _xpath2string(child, output, prefix + "\t");
  }
}

bool UI::xpath2string(Xpath* root, std::string& output) {
  _xpath2string(root, output, "");
  return true;
}


static void _xpath2json(UI::Xpath* root, Value& output, Allocator &al) {
  output.AddMember("tag", COM::toString(root->tag), al);
  output.AddMember("name", root->name, al);
  output.AddMember("class", root->clazz, al);

  if (!root->children.size())
    return;

  Value pool(rapidjson::kArrayType);
  for (UI::Xpath* child : root->children) {
    Value item(rapidjson::kObjectType);
    _xpath2json(child, item, al);
    pool.PushBack(item, al);
  }
  output.AddMember("children", pool, al);
}


bool UI::xpath2json(Xpath* root, Value& output, Allocator &al) {
  if (root->children.size() > 0) {
    _xpath2json(root->children[0], output, al);
    return true;
  }
  return false;
}


static
UI::Xpath* _travelXpath(UI::Xpath* path, std::vector<std::string>& routes, bool isRoot) {
    if (routes.size() <= 0)
        return path;

    std::string route = routes.front();
    routes.erase(routes.begin());

    LOGD("name:%s  route:%s", path->name.c_str(), route.c_str());

    if (isRoot) {
        /*
        if (path->name.compare(route) != 0)
            return nullptr;
            */
        return _travelXpath(path, routes, false);
    }

    if (!route.length() && routes.size() == 0)
        return path;

    for (UI::Xpath* child : path->children) {
      std::string pattern = child->name;
      if (route.length() && route[0] == '$')
        pattern = '$' + child->clazz;

      LOGD("pattern:%s  route:%s", pattern.c_str(), route.c_str());

      if (pattern.compare(route) == 0)
        return _travelXpath(child, routes, false);
    }
    return nullptr;
}

UI::Xpath* UI::travelXpath(UI::Xpath* root, std::string xpath) {
    std::vector<std::string> paths = COM::split(xpath, "/");
    return _travelXpath(root, paths, true);
}

void UI::click(Xpath* node) {
  static fun_1 click = (fun_1)(COM::alsr(FunAddrUIClick));
  void* instance = node->instance;
  LOGD("click instance:%p", instance);
  click((obj)instance);
}
