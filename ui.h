#ifndef __UI_H__
#define __UI_H__

#include "universal.h"
#include "common.h"

namespace UI {
    typedef struct Xpath {
        void*                     instance;
        std::string               clazz;
        std::string               name;
        int                       tag;
        std::vector<Xpath*>       children;
        bool                      enabled;
        bool                      free;
    } Xpath;

    Xpath* genXpath();
    void delXpath(Xpath* root);
    Xpath* travelXpath(UI::Xpath* root, std::string xpath);
    bool xpath2string(Xpath* root, std::string& output);
    bool xpath2json(Xpath* root, Value& output, Allocator &al);

    void click(Xpath* node);
}

#endif
