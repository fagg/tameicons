# tameicons

I got sick of Windows not restoring my desktop icons upon a resolution 
change. Typically this happens when I've RDP'ed into my main machine
from my laptop, and the resolution of the session has changed. So I
wrote this quick tool to make putting them back easier.

Maybe there's other ways to do it - I looked and couldn't find anything
sufficiently lightweight for my wants. So I wrote something. It's pretty
groaty but it works.

It is written to assume full 16bit unicode, so you should not have any
troubles with this even if your machine is set up in a language that's
not English.

To build, you run need to run:

`.\build.bat`

There are no external dependencies outside of the usual stuff - Windows API.

To save your current desktop icon layout:

`.\tameicons.exe /dump c:\some_file.bin`

To restore your desktop icon layout:

`.\tameicons.exe /restore c:\some_file.bin`

You can also use this to enumerate your desktop icons:

`.\tameicons.exe /show`

You will get a list of icons, including their names and positions.

```
Z:\>.\tameicons.exe /show
Name = Command Prompt, Pos = (13,2)
Name = Admin Command Prompt, Pos = (13,101)
```

Tested on Windows 11, but it should work on anything fairly recent.

## LICENSE

Copyright (c) 2023, Dr Ashton Fagg `<ashton@fagg.id.au>`
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 
3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
