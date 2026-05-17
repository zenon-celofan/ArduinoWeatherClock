#!/usr/bin/env python3
import os, sys, subprocess
from openai import OpenAI

client = OpenAI(
    base_url="https://openrouter.ai/api/v1",
    api_key=os.environ["OPENROUTER_API_KEY"],
)

guide = open("UserGuide.md").read()
readme = open("README.md").read()
ino   = open("ArduinoWeatherClock.ino").read()
eeprom_map = open("include/eeprom_map.h").read()

has_readme = bool(readme.strip())

prompt = f"""You are a technical doc reviewer. Compare UserGuide.md and README.md against the source code.

UserGuide.md:
{guide}

README.md:
{'[empty - needs initial content based on code]' if not has_readme else readme}

ArduinoWeatherClock.ino:
{ino}

include/eeprom_map.h:
{eeprom_map}

Check for: firmware version, hardware pins, config fields, API endpoints, display modes, features.
If both files are in sync with the code, reply with exactly:
NO_CHANGES

Otherwise, reply with sections for each file that needs updating.
Format:
=== UserGuide.md ===
<full updated content of UserGuide.md>

=== README.md ===
<full updated content of README.md>

Omit any file that doesn't need changes. Do NOT add explanations or markdown wrapping."""

resp = client.chat.completions.create(
    model="meta-llama/llama-3.3-70b-instruct:free",
    messages=[{"role": "user", "content": prompt}],
    temperature=0,
)
content = resp.choices[0].message.content.strip()

if content == "NO_CHANGES":
    print("Docs are in sync — no changes needed")
    sys.exit(0)

files_to_add = []
parts = content.split("===")
for part in parts:
    part = part.strip()
    if part.startswith("UserGuide.md"):
        path = "UserGuide.md"
        body = part.removeprefix("UserGuide.md").strip()
    elif part.startswith("README.md"):
        path = "README.md"
        body = part.removeprefix("README.md").strip()
    else:
        continue
    with open(path, "w") as f:
        f.write(body + "\n")
    files_to_add.append(path)
    print(f"Updated {path}")

if not files_to_add:
    print("No file updates parsed — exiting")
    sys.exit(1)

print(f"Committing {', '.join(files_to_add)}")
subprocess.run(["git", "config", "user.name", "github-actions"], check=True)
subprocess.run(["git", "config", "user.email", "github-actions@github.com"], check=True)
subprocess.run(["git", "add"] + files_to_add, check=True)
subprocess.run(["git", "commit", "-m", "docs: sync docs with code [skip ci]"], check=True)
subprocess.run(["git", "push"], check=True)
