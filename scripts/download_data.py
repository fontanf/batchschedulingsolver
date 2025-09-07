import gdown
import os
import pathlib

gdown.download(id="1lZXKceiw8z6ADkwm_Jct0gRefECxHCN0", output="data.7z")
os.system("7z x data.7z -o\"data\"")
pathlib.Path("data.7z").unlink()
