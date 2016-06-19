#pragma once
// only relevant on Windows!
#if defined (_WIN32) 
#  if defined (EXPORT_MASTER_API)
// when compiling for .DLL we must export
#    define MASTER_API    __declspec(dllexport)
#  else
// when using from other projects, we must import
#    define MASTER_API    __declspec(dllimport)
#  endif
#else
// on any other system we don't need it at all
#  define MASTER_API
#endif
