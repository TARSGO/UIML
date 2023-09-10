
#pragma once

#ifdef UIML_STRICT_MODE

#define UIML_CRASH_STRICT(reason)
#define UIML_FATAL_ASSERT(condition,reason) if(!(condition)) { UIML_CRASH_STRICT(reason); }

#else

#define UIML_CRASH_STRICT(reason)
#define UIML_FATAL_ASSERT(condition,reason)

#endif
