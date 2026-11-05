#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
cd "$(dirname "$0")"
bash ../../tools/sdk-container.sh make -j6 all "$@"
