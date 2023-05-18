import requests
from bs4 import BeautifulSoup as bs
import subprocess
import time
print("  _____          _               _ _              ______                  _____           _       _   ")
time.sleep(0.1)
print(" |  __ \        | |             (_) |            |  ____|                / ____|         (_)     | |  ")
time.sleep(0.1)
print(" | |__) | __ ___| |__  _ __ __ _ _| |_ ___ ______| |__ _ __ ___  ___    | (___   ___ _ __ _ _ __ | |_ ")
time.sleep(0.1)
print(" |  ___/ '__/ _ \ '_ \| '__/ _` | | __/ _ \______|  __| '__/ _ \/ _ \    \___ \ / __| '__| | '_ \| __|")
time.sleep(0.1)
print(" | |   | | |  __/ | | | | | (_| | | || (_) |     | |  | | |  __/  __/    ____) | (__| |  | | |_) | |_ ")
time.sleep(0.1)
print(" |_|   |_|  \___|_| |_|_|  \__,_| |\__\___/      |_|  |_|  \___|\___|   |_____/ \___|_|  |_| .__/ \__|")
time.sleep(0.1)
print("                               _/ |                                                        | |        ")
time.sleep(0.1)
print("                              |__/                                                         |_|        ")

addr = input("Enter video link: ")

def progress_bar(current, total, bar_length=20):
    fraction = current / total

    arrow = int(fraction * bar_length - 1) * '#'
    padding = int(bar_length - len(arrow)) * ' '

    ending = '\n' if current == total else '\r'

    print(f'Progress: [{arrow}{padding}] {int(fraction * 100)}%', end=ending)


def get_link_prehrajto(url):
    resp = requests.get(url)
    soup = bs(resp.text, "html5lib")
    js = str(soup.findAll("script"))

    for line in js.splitlines():
        if "https://storage" in line:
            line = line[line.find("h"):]
            line = line.split('"', 1)[0]
            return line


def get_video_title(url):
    resp = requests.get(url)
    soup = bs(resp.text, "html5lib")
    for i in soup.findAll("title"):
        return i.get_text().strip(" - online ke zhlédnutí a stažení - Přehraj.to")


def playVLC(source_url):
    p = subprocess.Popen(["C:/Program Files/VideoLAN/VLC/vlc.exe", source_url])


def download(source_url):
    with requests.get(source_url, stream=True) as r:
        r.raise_for_status()
        total_size = r.headers.get("Content-Length")
        with open(get_video_title(addr) + ".mp4", "wb") as f:
            percentage = 0
            for i, chunk in enumerate(r.iter_content(chunk_size=8192)):
                percentage_old = percentage
                percentage = int(i * 8192 / int(total_size) * 100)
                if not(percentage == percentage_old):
                    progress_bar(int(i * 8192), int(total_size))

                f.write(chunk)
                f.flush()

choice = int(input("[1] Download\n[2] Play with VLC\n"))

if(choice == 1):
    download(get_link_prehrajto(addr))
if (choice == 2):
  playVLC(get_link_prehrajto(addr))
