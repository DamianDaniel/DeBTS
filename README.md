# Debian BTS Client (DeBTS)

A GTK3 desktop client for the Debian Bug Tracking System. It talks to the
BTS entirely through your own IMAP/SMTP mail account - reading bug mail
over IMAP and sending new bugs, replies, and `control@bugs.debian.org`
commands over SMTP - so it needs nothing but the same mail access an email
client already has.

Styling is modeled on tracker.debian.org: crimson boxed panels, a plain
white page, and blue primary action buttons.

## Features

- **Bug list** grouped from your mail folder, with Bug#/Package/Severity/
  Status/Title columns, an All/Open/Done sidebar filter, and a "jump to bug
  #" box.
- **Bug detail view**: fetches and displays the full raw thread for a bug
  over IMAP, with one-click Reply and Control Commands actions.
- **File a new bug**: package chooser (editable dropdown of Debian's
  pseudo-packages, with an (i) button linking to Debian's explanation
  page), version, severity, title, and body - submitted to
  `submit@bugs.debian.org` with the right pseudo-headers.
- **Reply to a bug**: sends directly to `<bug>@bugs.debian.org`.
- **Control command builder**: every common `control@bugs.debian.org`
  command (retitle, reassign, severity, tags, usertags, merge, forcemerge,
  unmerge, clone, block/unblock, affects, forwarded, owner, submitter,
  archive, found/fixed and their inverses, close, reopen, ...) with a
  generated parameter form. Queue several commands and send them as one
  batch email, or hand-edit the raw batch text directly.
- **Settings**: IMAP/SMTP host, port, SSL, credentials, and which folder to
  scan for BTS mail, stored in `~/.config/debts/config.ini` (0600
  permissions, since it holds your mail password).

## Building

Dependencies: GTK3 and libcurl development headers.

```sh
# Debian/Ubuntu
sudo apt install build-essential libgtk-3-dev libcurl4-openssl-dev pkg-config

make
./debts
```

`make install` installs the binary to `/usr/bin/debts` and the stylesheet
to `/usr/share/debts/style.css` (respects `DESTDIR`).

## First-time setup

1. Launch the app, click the gear icon (Settings).
2. **Incoming (IMAP)** tab: your mail server, port (993 for IMAPS is the
   default), username/password, and which folder to scan for BTS traffic.
   It's worth setting up a mail filter that sorts mail from
   `*@bugs.debian.org` into its own folder and pointing this at that
   folder, so the bug list doesn't fill up with unrelated mail.
3. **Outgoing (SMTP)** tab: your mail server, port (587/STARTTLS or
   465/SMTPS), and credentials.
4. **Identity** tab: your name and, importantly, the email address you
   file bugs under - the BTS identifies you by `From:` address, not by
   password, so this needs to match the account you're sending through.
5. Save, then hit the refresh icon in the header bar.

## Known limitations

- **Bug status/package/severity are inferred from mail, not queried
  live.** The Debian BTS's authoritative state (current severity, tags,
  merges, etc.) lives on bugs.debian.org's server and is normally read via
  its web/SOAP interface, not IMAP. This client only sees what's arrived
  in your mail folder, and guesses status ("open" vs "done") heuristically
  from subject lines like "marked as done". Package/severity columns stay
  "?" until inferred from a `Package:`/`Severity:` pseudo-header appearing
  in the fetched raw text you've viewed, or from your own filed reports.
  For authoritative bug status, cross-check against bugs.debian.org.
- **Refresh re-scans the whole configured folder** on every click (no
  incremental sync/caching of read state).
- **Passwords are stored in plaintext** in `config.ini` (0600 permissions
  only). Consider an app-specific password if your provider supports one.
- Pseudo-package list in the New Bug dropdown is a curated subset of
  <https://www.debian.org/Bugs/pseudo-packages> - real package names can
  still be typed in directly, the field is a free-text combo.

## License

GPL-3.0-or-later (see the About dialog).
