"""
סקריפט שמוסיף את ספריית USB MSC של ST לבנייה.
הספרייה כבר קיימת בחבילת framework-stm32cubef4 שPlatformIO מוריד.
קבצי _template נמחרים — יש לנו מימוש משלנו ב-src/.
"""
Import("env")
import os, glob, sys

packages_dir = env.subst("$PROJECT_PACKAGES_DIR")
fw_dir  = os.path.join(packages_dir, "framework-stm32cubef4")
usb_dir = os.path.join(fw_dir, "Middlewares", "ST", "STM32_USB_Device_Library")

core_inc = os.path.join(usb_dir, "Core",        "Inc")
core_src = os.path.join(usb_dir, "Core",        "Src")
msc_inc  = os.path.join(usb_dir, "Class", "MSC","Inc")
msc_src  = os.path.join(usb_dir, "Class", "MSC","Src")

# בדיקת קיום הספרייה — עוזר לאבחון בעיות build
if not os.path.isdir(usb_dir):
    print("ERROR: USB Middleware not found at:", usb_dir, file=sys.stderr)
    print("       Run: pio pkg update", file=sys.stderr)
else:
    print("USB Middleware found:", usb_dir)

# הוסף include paths
env.Append(CPPPATH=[core_inc, msc_inc])

# קמפל רק קבצי Core שאינם template
core_files = [f for f in glob.glob(os.path.join(core_src, "*.c"))
              if "_template" not in os.path.basename(f)]

# קמפל רק קבצי MSC שאינם template
msc_files = [f for f in glob.glob(os.path.join(msc_src, "*.c"))
             if "_template" not in os.path.basename(f)]

print("USB Core sources:", [os.path.basename(f) for f in core_files])
print("USB MSC  sources:", [os.path.basename(f) for f in msc_files])

# בנה את האובייקטים פעם אחת ורשום אותם
objs = [env.Object(f) for f in core_files + msc_files]
env.Append(PIOBUILDFILES=objs)
