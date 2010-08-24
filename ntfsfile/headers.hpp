#pragma once

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <winioctl.h>
#include <shobjidl.h>
#include <imagehlp.h>
#include <softpub.h>

#include <comdef.h>
#include <process.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <algorithm>
using namespace std;

#include "lzo/lzo1x.h"
#include "lzo/lzo_asm.h"

#include "openssl/md4.h"
#include "openssl/md5.h"
#include "openssl/sha.h"

#include "plugin.hpp"
#include "farcolor.hpp"
#include "farkeys.hpp"

#include "col/AnsiString.h"
#include "col/UnicodeString.h"
#include "col/PlainArray.h"
#include "col/ObjectArray.h"
using namespace col;
