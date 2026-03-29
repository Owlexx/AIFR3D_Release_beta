#!/usr/bin/env node
const fs = require('fs');
const path = require('path');
const { chromium } = require('playwright');

const BASE_URL = process.env.UX_QA_BASE_URL || 'http://127.0.0.1:8787';
const OUT_DIR = process.env.UX_QA_OUT_DIR || path.join(process.cwd(), 'logs/qa', `website_${new Date().toISOString().replace(/[:.]/g, '-')}`);
const ADMIN_USER = String(process.env.UX_QA_ADMIN_USER || '').trim();
const ADMIN_PASS = String(process.env.UX_QA_ADMIN_PASS || '').trim();
const PROMO_CODE = String(process.env.UX_QA_PROMO_CODE || '').trim();

fs.mkdirSync(OUT_DIR, { recursive: true });
const uploadFile = path.join(OUT_DIR, 'qa_upload_sample.wav');
fs.writeFileSync(uploadFile, 'qa-sample-pack-audio');

const results = [];
function record(step, ok, detail = '') {
  results.push({ step, ok, detail });
  const icon = ok ? 'PASS' : 'FAIL';
  console.log(`[${icon}] ${step}${detail ? ` :: ${detail}` : ''}`);
}

async function screenshot(page, name) {
  const file = path.join(OUT_DIR, `${name}.png`);
  await page.screenshot({ path: file, fullPage: true });
}

async function run() {
  const browser = await chromium.launch({ headless: true });
  const context = await browser.newContext();

  await context.route('**/config.js', async (route) => {
    await route.fulfill({
      status: 200,
      contentType: 'application/javascript',
      body: `window.DAWAI_CONFIG = { apiBase: "${BASE_URL}", apiToken: "", wsBase: "${BASE_URL.replace(/^http/, 'ws')}/ws/chat" };`
    });
  });

  const page = await context.newPage();
  page.on('pageerror', (err) => {
    record('pageerror', false, err.message);
  });

  await page.goto(BASE_URL, { waitUntil: 'domcontentloaded' });
  await page.waitForSelector('#store');
  await page.waitForTimeout(1200);
  await screenshot(page, '00_home');
  record('load-home', true);

  const navTargets = ['#store', '#beats', '#services', '#soundpacks', '#support', '#policy', '#admin'];
  for (const target of navTargets) {
    try {
      await page.click(`nav a[href="${target}"]`);
      await page.waitForTimeout(250);
      const sectionVisible = await page.locator(target).isVisible();
      record(`nav-${target}`, sectionVisible, sectionVisible ? '' : 'section not visible');
      await screenshot(page, `nav_${target.replace('#', '')}`);
    } catch (error) {
      record(`nav-${target}`, false, error.message);
    }
  }

  const heroLinks = ['#store', '#beats'];
  for (const target of heroLinks) {
    try {
      await page.click(`.hero a[href="${target}"]`);
      await page.waitForTimeout(200);
      record(`hero-${target}`, true);
    } catch (error) {
      record(`hero-${target}`, false, error.message);
    }
  }

  await page.click('a[href="#beats"]');
  await page.waitForTimeout(200);

  try {
    await page.click('#catalog-toggle');
    await page.waitForTimeout(300);
    const openState = await page.getAttribute('#catalog-toggle', 'aria-expanded');
    const wrapVisible = await page.locator('#catalog-wrap').evaluate((el) => !el.hasAttribute('hidden'));
    record('beats-open-toggle', openState === 'true' && wrapVisible, `aria=${openState}`);

    await page.click('#catalog-toggle');
    await page.waitForTimeout(300);
    const closeState = await page.getAttribute('#catalog-toggle', 'aria-expanded');
    record('beats-close-toggle', closeState === 'false', `aria=${closeState}`);

    await page.click('#catalog-toggle');
    await page.waitForTimeout(300);
  } catch (error) {
    record('beats-toggle', false, error.message);
  }

  const trackButtons = page.locator('#catalog-list .track-card button');
  const trackCount = await trackButtons.count();
  record('catalog-track-load', trackCount > 0, `count=${trackCount}`);

  const playClicks = Math.min(trackCount, 3);
  for (let i = 0; i < playClicks; i += 1) {
    try {
      await trackButtons.nth(i).click();
      await page.waitForTimeout(300);
      const title = (await page.textContent('#now-title')) || '';
      record(`catalog-play-${i + 1}`, title.trim().length > 0 && title.trim() !== 'None', `title=${title.trim()}`);
    } catch (error) {
      record(`catalog-play-${i + 1}`, false, error.message);
    }
  }

  const controlButtons = ['#prev-track', '#next-track', '#toggle-track'];
  for (const selector of controlButtons) {
    try {
      await page.click(selector);
      await page.waitForTimeout(200);
      record(`player-${selector}`, true);
    } catch (error) {
      record(`player-${selector}`, false, error.message);
    }
  }

  try {
    const buyHref = await page.getAttribute('#buy-track', 'href');
    const ok = Boolean(
      buyHref &&
      (buyHref.startsWith('mailto:') || buyHref.includes('paypal.com') || buyHref.includes('venmo.com') || buyHref.startsWith('http'))
    );
    record('player-buy-link', ok, buyHref || 'missing');
  } catch (error) {
    record('player-buy-link', false, error.message);
  }

  await page.click('a[href="#support"]');
  await page.fill('#chat-input', 'qa sweep ping');
  await page.click('#chat-send');
  await page.waitForTimeout(1200);
  const chatMsgCount = await page.locator('#chat-log .msg').count();
  record('chat-send', chatMsgCount > 0, `messages=${chatMsgCount}`);

  try {
    await page.fill('#support-name', 'QA User');
    await page.fill('#support-email', 'qa@example.com');
    await page.fill('#support-message', 'Contact flow check');
    await page.click('#support-form button[type="submit"]');
    await page.waitForTimeout(300);
    const status = ((await page.textContent('#support-status')) || '').trim();
    record('support-form-send', status.toLowerCase().includes('sent') || status.toLowerCase().includes('saved'), `status=${status}`);
  } catch (error) {
    record('support-form-send', false, error.message);
  }

  await page.click('a[href="#admin"]');
  await page.waitForTimeout(300);
  await screenshot(page, 'admin_before_login');

  try {
    await page.selectOption('#soundpack-type', 'samplepack');
    await page.waitForTimeout(100);
    const samplePrice = ((await page.inputValue('#soundpack-price')) || '').trim();
    record('admin-price-samplepack', samplePrice === '$19.99', `value=${samplePrice}`);

    await page.selectOption('#soundpack-type', 'single');
    await page.waitForTimeout(100);
    const singlePrice = ((await page.inputValue('#soundpack-price')) || '').trim();
    record('admin-price-single', singlePrice === '$2.99', `value=${singlePrice}`);

    await page.selectOption('#soundpack-type', 'samplepack');
    await page.waitForTimeout(100);
  } catch (error) {
    record('admin-price-defaults', false, error.message);
  }

  try {
    await page.fill('#soundpack-title', 'QA No Login Pack');
    await page.fill('#soundpack-description', 'Should fail without login');
    await page.setInputFiles('#soundpack-file', uploadFile);
    await page.click('#admin-soundpack-form button[type="submit"]');
    await page.waitForTimeout(300);
    const status = ((await page.textContent('#soundpack-upload-status')) || '').trim();
    record('admin-upload-block-without-login', status.toLowerCase().includes('login required'), `status=${status}`);
  } catch (error) {
    record('admin-upload-block-without-login', false, error.message);
  }

  const canRunAdminFlow = Boolean(ADMIN_USER && ADMIN_PASS);
  if (!canRunAdminFlow) {
    record('admin-login', true, 'skipped: set UX_QA_ADMIN_USER and UX_QA_ADMIN_PASS');
  } else {
    try {
      await page.fill('#admin-username', ADMIN_USER);
      await page.fill('#admin-password', ADMIN_PASS);
      await page.click('#admin-login-form button[type="submit"]');
      await page.waitForTimeout(400);
      const adminStatus = ((await page.textContent('#admin-status')) || '').trim();
      record('admin-login', adminStatus.toLowerCase().includes('logged in as'), `status=${adminStatus}`);
    } catch (error) {
      record('admin-login', false, error.message);
    }
  }

  try {
    const hasOpenAiForm = await page.locator('#openai-config-form').isVisible();
    const hasOpenAiKey = await page.locator('#openai-api-key').isVisible();
    const hasOpenAiModel = await page.locator('#openai-model').isVisible();
    const hasOpenAiList = await page.locator('#openai-model-list').isVisible();
    record('admin-openai-form', hasOpenAiForm && hasOpenAiKey && hasOpenAiModel && hasOpenAiList);
  } catch (error) {
    record('admin-openai-form', false, error.message);
  }

  const qaPackTitle = `QA Pack ${Date.now()}`;
  if (!canRunAdminFlow) {
    record('admin-upload-pack', true, 'skipped: admin credentials not provided');
  } else {
    try {
      await page.fill('#soundpack-title', qaPackTitle);
      await page.fill('#soundpack-description', 'UX QA uploaded pack');
      await page.fill('#soundpack-bpm', '140');
      await page.fill('#soundpack-key', 'Am');
      await page.fill('#soundpack-tempo', '140');
      await page.fill('#soundpack-price', '$19.99');
      await page.setInputFiles('#soundpack-file', uploadFile);
      await page.click('#admin-soundpack-form button[type="submit"]');
      await page.waitForTimeout(700);

      const uploadStatus = ((await page.textContent('#soundpack-upload-status')) || '').trim();
      record('admin-upload-pack', uploadStatus.toLowerCase().includes('uploaded'), `status=${uploadStatus}`);
    } catch (error) {
      record('admin-upload-pack', false, error.message);
    }
  }

  await page.click('a[href="#soundpacks"]');
  await page.waitForTimeout(600);
  if (!canRunAdminFlow) {
    record('public-soundpack-visible', true, 'skipped: admin credentials not provided');
    record('soundpack-preview-buy', true, 'skipped: admin credentials not provided');
  } else {
    const hasQaPack = await page.locator(`#soundpack-list .track-card h4:text("${qaPackTitle}")`).count();
    record('public-soundpack-visible', hasQaPack > 0, `count=${hasQaPack}`);

    try {
      const qaCard = page.locator('#soundpack-list .track-card').filter({ hasText: qaPackTitle }).first();
      await qaCard.locator('button', { hasText: 'Preview' }).click();
      await page.waitForTimeout(300);
      const nowTitle = ((await page.textContent('#now-title')) || '').trim();
      record('soundpack-preview-buy', nowTitle.includes(qaPackTitle), `title=${nowTitle}`);
    } catch (error) {
      record('soundpack-preview-buy', false, error.message);
    }
  }

  await page.click('a[href="#store"]');
  await page.waitForTimeout(200);
  try {
    const hasPromoInput = await page.locator('#promo-code-input').isVisible();
    const hasPromoButton = await page.locator('#promo-redeem-button').isVisible();
    record('install-promo-ui', hasPromoInput && hasPromoButton);
  } catch (error) {
    record('install-promo-ui', false, error.message);
  }

  if (!PROMO_CODE) {
    record('install-promo-redeem', true, 'skipped: set UX_QA_PROMO_CODE');
  } else {
    try {
      await page.fill('#promo-code-input', PROMO_CODE);
      await page.click('#promo-redeem-button');
      await page.waitForTimeout(500);
      const status = ((await page.textContent('#promo-status')) || '').trim();
      record('install-promo-redeem', status.toLowerCase().includes('accepted'), `status=${status}`);
    } catch (error) {
      record('install-promo-redeem', false, error.message);
    }
  }

  const installLinks = await page.locator('#install-grid a').all();
  for (let i = 0; i < installLinks.length; i += 1) {
    const href = await installLinks[i].getAttribute('href');
    const text = ((await installLinks[i].textContent()) || '').trim();
    if (!href || href.startsWith('#')) {
      record(`install-link-${text || i + 1}`, true, href || 'hash-link');
      continue;
    }
    if (/^mailto:/i.test(href) || /venmo\.com/i.test(href) || /^https?:\/\/www\.paypal\.com/i.test(href)) {
      record(`install-link-${text || i + 1}`, true, `external=${href}`);
      continue;
    }
    try {
      const url = href.startsWith('http') ? href : `${BASE_URL}${href}`;
      const resp = await context.request.get(url);
      record(`install-link-${text || i + 1}`, resp.status() < 400, `status=${resp.status()} href=${href}`);
    } catch (error) {
      record(`install-link-${text || i + 1}`, false, error.message);
    }
  }

  await screenshot(page, 'final_state');

  const passCount = results.filter((r) => r.ok).length;
  const failCount = results.length - passCount;
  const summary = {
    timestamp: new Date().toISOString(),
    baseUrl: BASE_URL,
    outputDir: OUT_DIR,
    totals: { total: results.length, pass: passCount, fail: failCount },
    results
  };

  fs.writeFileSync(path.join(OUT_DIR, 'summary.json'), JSON.stringify(summary, null, 2));
  await browser.close();

  console.log(`\nSummary: ${passCount}/${results.length} passed, ${failCount} failed`);
  if (failCount > 0) {
    process.exit(1);
  }
}

run().catch((error) => {
  console.error(error);
  process.exit(1);
});
