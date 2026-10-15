# This script will clone the Ethereum and Exchange application that are needed for the Swap test setup
# Name "exchange" or "ethereum" to clone only that one
# Run the helper script helper_tool_build_dependencies.py after to build and dispatch them in the correct location

from _helper_tool import clone_and_pull_ethereum, clone_and_pull_exchange, selected_apps

apps = selected_apps("Clone the applications the swap tests need.")

if "exchange" in apps:
    clone_and_pull_exchange()
if "ethereum" in apps:
    clone_and_pull_ethereum()
