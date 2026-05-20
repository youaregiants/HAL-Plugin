Deploy the HAL server to the Hetzner VPS.

SSH to `platform@62.238.41.219` and run the deploy script:
```bash
ssh platform@62.238.41.219 "cd /home/platform/hal && ./deploy.sh"
```

Then verify the service is healthy:
```bash
ssh platform@62.238.41.219 "sudo systemctl status hal-api --no-pager -l && curl -s https://62-238-41-219.sslip.io/api/health"
```

Report the deploy output and health check result. If the service failed to start, fetch the last 30 lines of logs:
```bash
ssh platform@62.238.41.219 "journalctl -u hal-api -n 30 --no-pager"
```
