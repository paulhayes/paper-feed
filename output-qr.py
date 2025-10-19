Import("env")

import configparser
import hashlib

config = configparser.ConfigParser()
config.read("config.ini")

if env.IsIntegrationDump():
   # stop the current script execution
   Return()


try:
    import qrcode
    import PIL
except ImportError:
    env.Execute("$PYTHONEXE -m pip install qrcode pillow")

wifi_ssid=config['default']['wifi_ssid']
wifi_password=config['default']['wifi_password']
ota_password=config['default']['ota_password']
ota_hash=hashlib.md5(ota_password.encode()).hexdigest()
if env['UPLOAD_PROTOCOL']=='esptool':
    print("using esptool")

if "upload_protocol" in env and env['upload_protocol']=="espota":
    print("ota set")
    env.Replace(UPLOAD_FLAGS=[
        f"--auth={ota_password}"
    ])
env.Append(CPPDEFINES=[
    ('WIFI_SSID',env.StringifyMacro(wifi_ssid)),
    ('WIFI_PASS',env.StringifyMacro(wifi_password)),
    ('OTA_HASH',env.StringifyMacro(ota_hash))
])

qrcode_str = f"WIFI:T:WPA;S:{wifi_ssid};P:{wifi_password};;"
print(qrcode_str)
img = qrcode.make(qrcode_str)
type(img)  # qrcode.image.pil.PilImage
img.save("connect-qr.png")