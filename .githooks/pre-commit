#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

# Check git author and committer config
NAME="$(git config user.name || true)"
EMAIL="$(git config user.email || true)"

if [[ -z "$NAME" || -z "$EMAIL" ]]; then
    echo "ERROR: Git user.name or user.email is not set." >&2
    exit 1
fi

if [[ "$EMAIL" =~ @gmail\.com$ || "$EMAIL" =~ @yahoo\.com$ || "$EMAIL" =~ @hotmail\.com$ || "$EMAIL" =~ @outlook\.com$ ]]; then
    echo "ERROR: Personal email address detected in git config: $EMAIL" >&2
    echo "Please use a GitHub noreply email (e.g. username@users.noreply.github.com)." >&2
    exit 1
fi

# List of files staged for commit
STAGED_FILES=$(git diff --cached --name-only --diff-filter=ACM)

if [[ -z "$STAGED_FILES" ]]; then
    exit 0
fi

# Check forbidden file extensions/names
FORBIDDEN_FILES_PATTERN='(\.env$|\.env\.|\.pem$|\.key$|\.sqlite$|\.db$|CMakeCache\.txt)'
git diff --cached --name-only -z --diff-filter=ACM | while IFS= read -r -d '' file; do
    if echo "$file" | grep -Eq "$FORBIDDEN_FILES_PATTERN"; then
        echo "ERROR: Forbidden file type staged for commit: $file" >&2
        exit 1
    fi
done

# Check staged contents for secrets and local paths
FORBIDDEN_CONTENT_REGEX='(/home/[a-zA-Z0-9_-]+|/Users/[a-zA-Z0-9_-]+|C:\\Users\\[a-zA-Z0-9_-]+|BEGIN (RSA|OPENSSH|EC|PGP) PRIVATE KEY|ghp_[A-Za-z0-9_]{36}|github_pat_[A-Za-z0-9_]{82}|AKIA[0-9A-Z]{16})'

FOUND_VIOLATION=0
while IFS= read -r -d '' file; do
    # Skip self and hook files
    if [[ "$file" == "tools/git/pre-commit.sh" || "$file" == ".githooks/pre-commit" ]]; then
        continue
    fi

    # Skip binary files
    if file -b --mime-encoding "$file" | grep -q 'binary'; then
        continue
    fi

    # Check for hardcoded paths or secrets in staged additions
    MATCHES=$(git diff --cached -- "$file" | grep -E "^\+" | grep -Ev "^\+\+\+" | grep -E "$FORBIDDEN_CONTENT_REGEX" || true)
    if [[ -n "$MATCHES" ]]; then
        echo "ERROR: Forbidden pattern (private path or potential secret) detected in staged file $file:" >&2
        echo "$MATCHES" >&2
        FOUND_VIOLATION=1
    fi
done < <(git diff --cached --name-only -z --diff-filter=ACM)

if [[ $FOUND_VIOLATION -ne 0 ]]; then
    echo "Commit aborted due to privacy or security rule violation." >&2
    exit 1
fi

exit 0
