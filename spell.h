#ifndef __SPELL_H__
#define __SPELL_H__

#include "universal.h"
#include "common.h"


namespace Spell {
  Variant* getAll();
  void use(int spellId, signed char isSelf=-1);
}

#endif
