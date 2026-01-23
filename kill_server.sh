#!/usr/bin/env bash

ps aux | grep "./portal" | awk '{print $2}' | tail -2 | head -1 | xargs kill -SIGINT
