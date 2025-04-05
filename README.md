# mac-keylogger-c

A simple macOS keylogger written in pure C. It captures all keystrokes system-wide using macOS's built-in event tap API — no Objective-C, no extra libraries. Great for learning how input hooks work on macOS. Works on Ventura, Monterey, and newer. Lightweight and easy to run at startup with launchd.

## Compilation

```
clang -o keylogger main.c -framework ApplicationServices
```

