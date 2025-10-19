#ifndef PACLIST_H
#define PACLIST_H

#ifdef __APPLE__
#include "OSX/osx-handler.h"
#define GETPACKAGES()		(osx_pkgs())
#endif

#ifdef __linux__
#include "ARCH/arch-handler.h"
#define GETPACKAGES()		(arch_pkgs())
#endif

#endif
