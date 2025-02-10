Import("env")

if env.IsIntegrationDump():
   # stop the current script execution
   Return()

#env.Execute("$PYTHONEXE -m pip install qrcode")

# [env]
# build_flags =
#   -DMWIFI_SSID=\"CaptivePortalTest\"
#   -DMWIFI_PASS="\"wicked stallion grape goose\""
# extra_scripts = output-qr.py


# print(env.Dump())

try:
    import qrcode
    import PIL
except ImportError:
    env.Execute("$PYTHONEXE -m pip install qrcode pillow")

ssid = 'CaptivePortalTest'
password = 'wicked stallion grape goose'

env.Append(CPPDEFINES=[
    ('WIFI_SSID',env.StringifyMacro(ssid)),
    ('WIFI_PASS',env.StringifyMacro(password))
])

img = qrcode.make(f"WIFI:T:WPA;S:{ssid};P:{password};;")
type(img)  # qrcode.image.pil.PilImage
img.save("connect-qr.png")