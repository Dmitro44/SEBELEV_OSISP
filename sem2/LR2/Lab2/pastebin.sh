#!/usr/bin/env bash
# Simple paste client using https://paste.rs
# Supports creating a paste from stdin or file, optional language/type

set -euo pipefail

usage() {
    cat <<EOF
Usage: $0 [options] [id|file]
Create new paste from stdin or file, or download by id.

Options:
  -f FILE     read input from FILE (upload)
  -l LANG     language for highlighting (metadata). Auto-detected from file ext if omitted
  -t TYPE     type: text|code (default: text). If code and LANG given it'll wrap in fenced block
  -o FILE     output downloaded paste into FILE (download)
  -h          show this help

Examples:
  echo testing | $0 -t text
  $0 -f script.py -t code    # upload file
  $0 QEXe0syg                 # download paste with id
EOF
}

detect_lang() {
    local fname="$1"
    case "${fname##*.}" in
    py) echo python ;;
    sh) echo bash ;;
    js) echo javascript ;;
    ts) echo typescript ;;
    c) echo c ;;
    cpp | cc | cxx | hpp) echo cpp ;;
    java) echo java ;;
    rb) echo ruby ;;
    php) echo php ;;
    html | htm) echo html ;;
    css) echo css ;;
    json) echo json ;;
    go) echo go ;;
    rs) echo rust ;;
    lua) echo lua ;;
    md) echo markdown ;;
    *) echo "" ;;
    esac
}

TYPE=text
LANG=""
INFILE=""
OUTFILE=""

while getopts ":f:l:t:o:h" opt; do
    case $opt in
    f) INFILE="$OPTARG" ;;
    l) LANG="$OPTARG" ;;
    t) TYPE="$OPTARG" ;;
    o) OUTFILE="$OPTARG" ;;
    h)
        usage
        exit 0
        ;;
    \?)
        echo "Unknown option: -$OPTARG" >&2
        usage
        exit 2
        ;;
    esac
done
shift $((OPTIND - 1))

# If a positional argument remains and -f not used, it may be an existing file (upload) or an id (download)
if [ -n "${1-}" ]; then
    if [ -z "$INFILE" ] && [ -f "$1" ]; then
        INFILE="$1"
    else
        ID_OR_ARG="$1"
    fi
fi

# If downloading: ID provided (or URL)
if [ -n "${ID_OR_ARG-}" ] && [ -z "$INFILE" ]; then
    # treat as id or url
    if [[ "$ID_OR_ARG" =~ ^https?:// ]]; then
        URL="$ID_OR_ARG"
    else
        URL="https://paste.rs/$ID_OR_ARG"
    fi
    if [ -n "$OUTFILE" ]; then
        # download to temp, optionally strip fenced code blocks if outfile extension matches
        TMPD=$(mktemp)
        if ! curl -sSf "$URL" -o "$TMPD"; then
            echo "Download failed" >&2
            rm -f "$TMPD"
            exit 1
        fi
        OUTLANG=$(detect_lang "$OUTFILE")
        # read first and last lines to detect fenced code block
        firstline=$(sed -n '1p' "$TMPD" || true)
        stripped=0
        # Match fence with language: ```lang
        if echo "$firstline" | grep -E -q '^[[:space:]]*```[[:space:]]*$'; then
            # fence without lang
            if [ -n "$OUTLANG" ]; then
                sed '1d;$d' "$TMPD" > "$OUTFILE"
                stripped=1
            fi
        else
            if echo "$firstline" | grep -E -q '^[[:space:]]*```[[:alnum:]+_.-]+[[:space:]]*$'; then
                fenced_lang=$(echo "$firstline" | sed -E 's/^[[:space:]]*```([[:alnum:]+_.-]+)[[:space:]]*$/\1/')
                if [ -n "$OUTLANG" ] && [ "$OUTLANG" = "$fenced_lang" ]; then
                    sed '1d;$d' "$TMPD" > "$OUTFILE"
                    stripped=1
                fi
            fi
        fi
        if [ $stripped -eq 0 ]; then
            mv "$TMPD" "$OUTFILE"
        else
            rm -f "$TMPD"
        fi
    else
        curl -sSf "$URL"
    fi
    exit $?
fi

# Create new paste
# determine language if not set and file provided
if [ -z "$LANG" ] && [ -n "$INFILE" ]; then
    LANG=$(detect_lang "$INFILE")
fi

# Prepare content (possibly wrapped) into temp file then send
TMP=$(mktemp)
trap 'rm -f "$TMP"' EXIT

if [ -n "$INFILE" ]; then
    if [ "$TYPE" = "code" ] && [ -n "$LANG" ]; then
        printf '```%s\n' "$LANG" > "$TMP"
        cat "$INFILE" >> "$TMP"
        printf '\n```\n' >>"$TMP"
    else
        cat "$INFILE" >"$TMP"
    fi
else
    # read from stdin
    if [ "$TYPE" = "code" ] && [ -n "$LANG" ]; then
        printf '```%s\n' "$LANG" > "$TMP"
        cat - >> "$TMP"
        printf '\n```\n' >>"$TMP"
    else
        cat - >"$TMP"
    fi
fi

# send to paste.rs
RESP=$(curl -sS --data-binary "@${TMP}" https://paste.rs/)
if [ -z "$RESP" ]; then
    echo "Upload failed or empty response" >&2
    exit 1
fi

# paste.rs returns a URL like https://paste.rs/<id>
# print id to stdout
ID=$(basename "$RESP")
echo "$ID"

exit 0
