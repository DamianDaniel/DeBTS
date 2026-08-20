# Debian BTS Client (DeBTS)

A GTK3 desktop client for the Debian Bug Tracking System. It sends bug
reports, replies, and `control@bugs.debian.org` commands over your own
IMAP/SMTP mail account - and for reading and searching, it opens the real
bugs.debian.org in an embedded browser tab, so you're always looking at
the actual site, not a scraped copy.

Styling is made to be like tracker.debian.org

## Features

- **First run**: log in with your mail account, or continue as a guest
  (guest mode can still browse and search, it just can't send mail).
- **Bug list**: bugs found in your mail folder, plus (once logged in)
  bugs tied to your email pulled from the public UDD mirror
  (udd-mirror.debian.net) a structured, public copy of Debian's
  own bug database, not scraped HTML, with an All/Open/Done filter and
  a "jump to bug #" box.
- **Browse BTS tab**: a real embedded browser (WebKit) pointed at
  bugs.debian.org. Search results, bug pages, everything, it's the
  actual site. When the page you're on is a bug report, a floating
  toolbar appears with Reply, Retitle, Severity, and More Commands
  buttons that open the matching native form, pre-filled with that bug's
  number.
- **Search window**: the same filters pkgreport.cgi supports (package,
  source, maintainer, submitter, severity, status, tag, owner) hitting
  Search opens the real results page in the Browse tab.
- **File a new bug**: package chooser (editable dropdown of Debian's
  pseudo-packages, with an (i) button linking to Debian's explanation
  page), version, severity, title, body, and a template picker (bug
  report, feature request, FTBFS, ITP, RFP, orphan) - submitted to
  `submit@bugs.debian.org`.
- **Control command builder**: every common `control@bugs.debian.org`
  command (retitle, reassign, severity, tags, usertags, merge,
  forcemerge, unmerge, clone, block/unblock, affects, forwarded, owner,
  submitter, archive, found/fixed and their inverses, close, reopen,
  ...). Queue several commands and send them as one batch email, or
  hand-edit the raw text directly.
- **Settings**: IMAP/SMTP host, port, SSL, credentials, and which folder
  to scan for BTS mail, stored in `~/.config/debts/config.ini` (0600
  permissions, since it holds your mail password).

## Building

Dependencies: GTK3, WebKit2GTK 4.1 (or 4.0), libcurl, and libpq
(PostgreSQL client library, used for UDD queries).

```sh
# Debian/Ubuntu
sudo apt install build-essential libgtk-3-dev libwebkit2gtk-4.1-dev \
                  libcurl4-openssl-dev libpq-dev pkg-config

make
./debts
```

`make` checks for all of these up front and tells you exactly what's
missing and how to install it, rather than failing partway through with
a wall of header errors.

`make install` installs the binary to `/usr/bin/debts` and the stylesheet
to `/usr/share/debts/style.css` (respects `DESTDIR`).

## First-time setup

On first launch you'll see a **Log In With Email** / **Continue as Guest**
screen. Guest mode is enough to browse and search bugs.debian.org; logging
in also lets you send mail and see bugs tied to your address.

If you log in or open Settings later:

1. **Incoming (IMAP)** tab: your mail server, port (993/IMAPS by
   default), username/password, and which folder to scan for BTS
   traffic. It's worth filtering mail from `*@bugs.debian.org` into its
   own folder and pointing this there.
2. **Outgoing (SMTP)** tab: your mail server, port (587/STARTTLS or
   465/SMTPS), and credentials.
3. **Identity** tab: your name and, importantly, the email address you
   file bugs under - the BTS identifies you by `From:` address, not by
   password, so this needs to match the account you're sending through.

## Known limitations

- **Bug status/package/severity in the local bug list are best-effort
  where they come from your own mail folder**, guessed heuristically
  from subject lines like "marked as done". Entries pulled in because
  you're logged in come from the public UDD mirror and are accurate,
  structured data straight from Debian's own bug database - no
  scraping involved.
- **Refresh re-scans the whole configured folder** on every click (no
  incremental sync/caching of read state).
- **Passwords are stored in plaintext** in `config.ini` (0600 permissions
  only). Consider an app-specific password if your provider supports one.
- Pseudo-package list in the New Bug dropdown is a curated subset of
  <https://www.debian.org/Bugs/pseudo-packages> - real package names can
  still be typed in directly, the field is a free-text combo.
- The embedded browser disables GPU-accelerated compositing
  (`WEBKIT_DISABLE_COMPOSITING_MODE=1` plus the matching WebKitSettings
  policy), since accelerated compositing has been a source of hard
  crashes on some GPU driver stacks. If you still hit a crash, a gdb
  backtrace (`gdb -ex run -ex bt --args ./debts`) is the fastest way to
  track it down further.

## License

GPL-3.0 (see `LICENSE`). Copyright (C) 2026 Damian Daniel
<damian@danielovci.net>.
