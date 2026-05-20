Check the status of all HAL services on the VPS.

Run these checks:
```bash
ssh platform@62.238.41.219 "
  echo '=== hal-api service ===' &&
  sudo systemctl status hal-api --no-pager -l &&
  echo '=== health endpoint ===' &&
  curl -s https://62-238-41-219.sslip.io/api/health &&
  echo '=== Caddy ===' &&
  sudo systemctl status caddy --no-pager | head -5 &&
  echo '=== disk ===' &&
  df -h / | tail -1 &&
  echo '=== memory ===' &&
  free -h | grep Mem
"
```

Summarize: is the API up? Is Caddy proxying? Any resource concerns?
