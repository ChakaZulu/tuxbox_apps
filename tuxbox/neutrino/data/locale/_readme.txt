Format of .locale files:
------------------------
character encoding: UTF-8
filename suffix   : .locale


Destination of .locale files:
-----------------------------
directory: /var/tuxbox/config/locale or /share/tuxbox/neutrino/locale


Verfication of .locale files:
-----------------------------
Use the check.locale.files shell script to figure out
- violations of the sorting order,
- missing translations and
- legacy strings.
deutsch.locale is considered being the most recent locale file.


Useful tools:
-------------
- emacs (add '(file-coding-system-alist (quote (("\\.locale\\'" . utf-8-unix) ("" undecided)))) to .emacs or use "C-x <RET> c utf-8 <RET> C-x C-f deutsch.locale")
- iconv
- sort
- uxterm
