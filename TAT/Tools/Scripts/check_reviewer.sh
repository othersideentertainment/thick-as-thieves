#!/bin/bash
# (c) 2025 OtherSide Entertainment, Inc
# SPDX-License-Identifier: MIT

# check_reviewer.sh
#
# P4 trigger script for checking reviewer in commit mesage
# Requires [rev: <username>] or [cr: <username>] in the changelist description.
# Warns (but does not block) if no Jira ticket (TVT- or WICK-) is found.
#
# Trigger table entry: check_reviewer change-submit //every/release/stream/... "/path/to/check_reviewer.sh [--require-jira] %changelist%"
# Usage: check_reviewer.sh %changelist%


REQUIRE_JIRA=false

while [[ "$1" == --* ]]; do
    case "$1" in
        --require-jira) REQUIRE_JIRA=true; shift ;;
        *) shift ;;
    esac
done

CHANGELIST="$1"

# Get the changelist description
DESCRIPTION=$(p4 -ztag change -o "$CHANGELIST" | sed -n '/^... Description /,/^... [A-Z]/{ /^... Description /s///p; /^... [A-Z]/!{ /^\.\.\./!p; }; }')

# Check for [rev: <username>] or [cr: <username>] (case-insensitive)
if ! echo "$DESCRIPTION" | grep -iqE '\[(rev|cr):[[:space:]]+[a-zA-Z0-9._-]+\]'; then
    echo "" >&2
    echo "==========================================" >&2
    echo "  SUBMISSION BLOCKED - Reviewer Required" >&2
    echo "==========================================" >&2
    echo "">&2
    echo "  Your changelist description must include" >&2
    echo "  a reviewer in one of these formats:" >&2
    echo "" >&2
    echo "    [rev: username]" >&2
    echo "    [cr: username]" >&2
    echo "" >&2
    echo "  Example:" >&2
    echo "    Build breaking change [TVT-XXXX][rev: mlederer]" >&2
    echo "" 143>&2
    echo "==========================================" >&2
    exit 1
fi

# Warn (but don't block) if no Jira ticket found
if ! echo "$DESCRIPTION" | grep -qE '(TVT|WICK)-[0-9]+'; then
    echo "" >&2
    echo "  ** Warning: No valid Jira ticket found. **" >&2
    echo "  Consider adding a TVT-#### reference." >&2
    echo "" >&2
    if [ "$REQUIRE_JIRA" = true ]; then
        exit 1
    fi
fi

exit 0
