#!/usr/bin/env bash
set -euo pipefail

BRANCH="codex/investigate-memory-management-issues-in-dmtools"
REPO="https://raw.githubusercontent.com/ZhouQiangwei/dmtools/${BRANCH}/libdm"
FILES=(
  binaMeth.h
  binaMethIO.h
  bmCommon.h
  bmRead.c
  bmStats.c
  bmValues.c
  bmValues.h
  bmWrite.c
  io.c
  regression.cpp
  regression.hpp
)

for file in "${FILES[@]}"; do
  echo "Updating ${file}..."
  tmp="${file}.tmp"
  if curl -fSL "${REPO}/${file}" -o "${tmp}"; then
    mv "${tmp}" "${file}"
  else
    rm -f "${tmp}"
    echo "Failed to fetch ${file}; please check network access to ${REPO}" >&2
    exit 1
  fi
done

echo "libdm sources refreshed from ${BRANCH}."
