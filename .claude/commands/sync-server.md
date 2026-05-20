Sync local server file changes to the VPS and restart the API.

This is used when you've edited files under a local `server/` directory or made changes you want to push to `/home/platform/hal/` on the VPS.

Steps:
1. Show a diff of what changed: `git diff HEAD -- server/` (if server/ exists locally) or describe what was changed in this session
2. SCP or rsync changed files to the VPS:
```bash
rsync -avz --exclude='__pycache__' --exclude='*.pyc' --exclude='.env' \
  server/ platform@62.238.41.219:/home/platform/hal/
```
3. Restart the service:
```bash
ssh platform@62.238.41.219 "sudo systemctl restart hal-api && sleep 2 && curl -s https://62-238-41-219.sslip.io/api/health"
```

Report what was synced and whether the health check passed.
