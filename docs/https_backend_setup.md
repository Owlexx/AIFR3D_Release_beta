# HTTPS Backend Setup (No Cloudflare API Required)

## What This Does
- Runs DawAI backend locally on port `8787`.
- Uses Caddy to provide HTTPS for:
  - `https://north3rnlight3r.com`
  - `https://www.north3rnlight3r.com`
  - `https://api.north3rnlight3r.com`
- Keeps payments and chat on your own backend.

## 1) Start Backend
```bash
cd /home/north3rnlight3r/Documents/DawAI_Ecosystem
./scripts/run_backend_production.sh
```

## 2) Install Caddy (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install -y caddy
```

## 3) Apply Caddy Config
```bash
sudo cp /home/north3rnlight3r/Documents/DawAI_Ecosystem/deploy/Caddyfile.north3rnlight3r.example /etc/caddy/Caddyfile
sudo systemctl restart caddy
sudo systemctl status caddy --no-pager
```

## 4) DNS / Router Requirements
- DNS `A` records must point to your public IP:
  - `north3rnlight3r.com` -> your public IP
  - `www.north3rnlight3r.com` -> your public IP
  - `api.north3rnlight3r.com` -> your public IP
- Router port forward:
  - `80 -> 10.90.109.29`
  - `443 -> 10.90.109.29`

## 5) Verify
```bash
curl -I https://north3rnlight3r.com
curl -s https://north3rnlight3r.com/api/v1/health
curl -s -X POST https://north3rnlight3r.com/api/v1/pay/create-order \
  -H 'Content-Type: application/json' \
  -d '{"sku":"software_vst3"}'
```

## 6) Webhook in PayPal Live App
- URL:
  - `https://north3rnlight3r.com/api/v1/pay/webhook`
- Events:
  - `CHECKOUT.ORDER.APPROVED`
  - `PAYMENT.CAPTURE.COMPLETED`

Save the webhook and keep the Webhook ID in your notes.
