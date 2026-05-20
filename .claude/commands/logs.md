Stream the last 50 lines of HAL API server logs from the VPS.

```bash
ssh platform@62.238.41.219 "journalctl -u hal-api -n 50 --no-pager"
```

Look for errors, cost tracking lines (input_tokens/output_tokens/cost_usd), cache hits, and any generation failures. Summarize what you see.
