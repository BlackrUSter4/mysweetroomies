#include "LayerData.h"

// Out of line so the vtable and typeinfo (needed by instanceof / dynamic_pointer_cast) live in one object file.
LayerData::~LayerData() {}
