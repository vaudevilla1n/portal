#!/usr/bin/env bash

ss -tunl | grep 9166

ps aux | grep "./portal"
