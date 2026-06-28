#!/usr/bin/env python3
import sys
import os
import json
import urllib.request
import urllib.error

# Offline rule-based translations for quick offline demo/fallback
OFFLINE_INTENT_RULES = [
    {
        "keywords": ["convert", "video", "1080p"],
        "command": "ffmpeg -i input.mp4 -vf scale=1920:1080 -c:v libx264 output.mp4",
        "description": "Convert video to 1080p using ffmpeg"
    },
    {
        "keywords": ["compress", "images"],
        "command": "find . -type f \\( -name \"*.jpg\" -o -name \"*.png\" -o -name \"*.jpeg\" \\) -exec convert {} -quality 80 {} \\;",
        "description": "Compress all JPG and PNG images in the current folder using ImageMagick"
    },
    {
        "keywords": ["find", "biggest", "files"],
        "command": "du -ah . | sort -rh | head -n 10",
        "description": "Find the 10 largest files/directories in the current folder"
    },
    {
        "keywords": ["make", "website"],
        "command": "npx -y create-vite@latest . --template react && npm install && npm run dev",
        "description": "Create a React/Vite website in the current directory and start dev server"
    }
]

def get_api_key():
    return os.environ.get("GEMINI_API_KEY") or os.environ.get("OPENAI_API_KEY")

def call_gemini(prompt):
    api_key = get_api_key()
    if not api_key:
        return None
    
    # We use Gemini API (gemini-1.5-flash or gemini-2.5-flash) via a standard REST call
    url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key={api_key}"
    
    headers = {
        "Content-Type": "application/json"
    }
    
    data = {
        "contents": [
            {
                "parts": [
                    {"text": prompt}
                ]
            }
        ]
    }
    
    req = urllib.request.Request(url, data=json.dumps(data).encode("utf-8"), headers=headers, method="POST")
    try:
        with urllib.request.urlopen(req) as response:
            res_data = json.loads(response.read().decode("utf-8"))
            # Extract content from response
            try:
                candidate = res_data["candidates"][0]
                text = candidate["content"]["parts"][0]["text"]
                return text.strip()
            except (KeyError, IndexError):
                return None
    except urllib.error.URLError as e:
        sys.stderr.write(f"API Request failed: {e}\n")
        return None

def handle_intent(intent, cwd):
    # Try offline matching first
    intent_lower = intent.lower()
    for rule in OFFLINE_INTENT_RULES:
        # Check if all keywords are in the intent
        if all(kw in intent_lower for kw in rule["keywords"]):
            print(rule["command"])
            return
            
    # Try online API translation
    api_key = get_api_key()
    if api_key:
        prompt = (
            "You are a translation engine inside a custom shell called RazzShell. "
            "Convert the user's natural language command intent into a valid, single-line shell command. "
            "Do NOT output any markdown blocks, backticks, or comments. Output ONLY the raw executable command.\n\n"
            f"Current directory: {cwd}\n"
            f"User Intent: {intent}\n"
            "Command: "
        )
        cmd = call_gemini(prompt)
        if cmd:
            # Clean up if AI output markdown formatting
            if cmd.startswith("```"):
                cmd = cmd.split("\n", 1)[1]
            if cmd.endswith("```"):
                cmd = cmd.rsplit("\n", 1)[0]
            if cmd.startswith("bash"):
                cmd = cmd[4:].strip()
            print(cmd.strip())
            return

    # No match and no API key
    sys.stderr.write("No matching offline command, and GEMINI_API_KEY is not set.\n")
    sys.stderr.write("Try setting GEMINI_API_KEY environment variable.\n")
    sys.exit(1)

def handle_explain(failed_cmd, error_log_path, cwd):
    error_content = ""
    if os.path.exists(error_log_path):
        try:
            with open(error_log_path, "r", encoding="utf-8", errors="ignore") as f:
                # Read last 100 lines to avoid prompt size limits
                lines = f.readlines()
                error_content = "".join(lines[-100:])
        except Exception as e:
            error_content = f"Could not read error log: {e}"

    api_key = get_api_key()
    if api_key:
        prompt = (
            "You are a senior system administrator and debugger in RazzShell.\n"
            f"The command '{failed_cmd}' failed in directory '{cwd}'.\n"
            f"Output/Errors:\n---\n{error_content}\n---\n"
            "Provide a concise explanation of why it failed (max 3 bullet points) "
            "and propose a fixed command. Present the explanation beautifully using ANSI colors if desired."
            "Format the proposed fix as: 'FIX: <fixed command>' on the last line."
        )
        explanation = call_gemini(prompt)
        if explanation:
            print(explanation)
            return

    # Offline generic fallback
    print(f"\033[1;31mCommand failed:\033[0m {failed_cmd}")
    print("\nCommon failure reasons:")
    print("1. Command not installed or not in PATH")
    print("2. Incorrect syntax or missing arguments")
    print("3. Permission denied (requires sudo)")
    print("\nTo enable smart AI debugging, set the \033[1;36mGEMINI_API_KEY\033[0m environment variable.")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: ai_helper.py --intent <prompt> <cwd> or --explain <failed_cmd> <log_file> <cwd>")
        sys.exit(1)
        
    mode = sys.argv[1]
    if mode == "--intent":
        handle_intent(sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else os.getcwd())
    elif mode == "--explain":
        handle_explain(sys.argv[2], sys.argv[3], sys.argv[4] if len(sys.argv) > 4 else os.getcwd())
    else:
        print(f"Unknown mode: {mode}")
        sys.exit(1)
