# Base64 Library

This directory contains a vendored Base64 encode/decode implementation used by
the OpenAquatix firmware.

Source:
- https://web.mit.edu/freebsd/head/contrib/wpa/src/utils/

Upstream component:
- `base64.c`
- `base64.h`
- supporting headers copied with the import

Original author:
- Jouni Malinen `<j@w1.fi>`

Copyright:
- `base64.c`: Copyright (c) 2005-2011, Jouni Malinen
- `base64.h`: Copyright (c) 2005, Jouni Malinen
- `includes.h`: Copyright (c) 2005-2006, Jouni Malinen
- `os.h`: Copyright (c) 2005-2009, Jouni Malinen
- `build_config.h`: Copyright (c) 2005-2006, Jouni Malinen

License:
- BSD license, as stated in the upstream source file headers

Notes:
- This is a vendored copy of the upstream implementation from the FreeBSD-hosted
  `wpa_supplicant/hostapd` utility sources.