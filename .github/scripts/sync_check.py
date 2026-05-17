#!/usr/bin/env python3
import os, sys, subprocess
from openai import OpenAI

client = OpenAI(
    base_url="https://openrouter.ai/api/v1",
    api_key=os.environ["OPENROUTER_API_KEY"],
)

guide = open("UserGuide.md").read()
ino   = open("ArduinoWeatherClock.ino").read()
eeprom_map = open("include/eeprom_map.h").read()

prompt = f"""You are a technical doc reviewer. Compare the UserGuide.md against the source code and EEPROM map below.

UserGuide.md:
{guide}

ArduinoWeatherClock.ino:
{ino}

include/eeprom_map.h:
{eeprom_map}

If UserGuide.md is fully in sync with the code, reply with exactly:
NO_CHANGES

If there are discrepancies, reply with the COMPLETE updated UserGuide.md content.
Do NOT add any explanation or markdown wrapping — just the file content."""

resp = client.chat.completions.create(
    model="baidu/cobuddy:free",
    messages=[{"role": "user", "content": prompt}],
    temperature=0,
)
content = resp.choices[0].message.content.strip()

if content == "NO_CHANGES":
    print("UserGuide.md is in sync — no changes needed")
    sys.exit(0)

with open("UserGuide.md", "w") as f:
    f.write(content)

print("UserGuide.md updated — committing")
subprocess.run(["git", "config", "user.name", "github-actions"], check=True)
subprocess.run(["git", "config", "user.email", "github-actions@github.com"], check=True)
subprocess.run(["git", "add", "UserGuide.md"], check=True)
subprocess.run(["git", "commit", "-m", "docs: sync UserGuide.md with code [skip ci]"], check=True)
subprocess.run(["git", "push"], check=True)
