# DisplaySurface configuration

`config.ini` is retained for upcoming DisplaySurface configuration work. It
currently records workspace width and height.

The assembly source does not read this file yet. At startup, `ds_0` currently
uses the primary monitor's detected bounds, so these INI values do not control
the presented extent. Configuration loading can be connected when that part
of the DisplaySurface work resumes.

`layout_en_us_brackets.inc` is a separate, data-only English/US virtual-key
map for the temporary bracket-rune test. It does not change or replace
`config.ini`.
