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



How do I add a new locale string?
---------------------------------
1.)
First of all, add the new string to deutsch.locale and english.locale while preserving their sorting order.
Do not add any empty lines.
Use apps/tuxbox/neutrino/data/locale/check.locale.files for verification purposes (see section above).

2.)
Enter the directory apps/tuxbox/neutrino/data/locale.

3.)
Create replacement files for
apps/tuxbox/neutrino/src/system/locals.h
and
apps/tuxbox/neutrino/src/system/locals_intern.h
using the following commands (note that they are different):
cut -d' ' -f1 deutsch.locale | sort | uniq | tr [:lower:] [:upper:] | tr \. \_  | tr \- \_ | tr -d \? | ./create.locals.h
cut -d' ' -f1 deutsch.locale | sort | uniq | ./create.locals_intern.h

4.)
Check if the modifications are correct:
diff locals.h ../../src/system/locals.h
diff locals_intern.h ../../src/system/locals_intern.h

5.)
Copy the replacement file to their destination
cp -p locals.h ../../src/system/locals.h
cp -p locals_intern.h ../../src/system/locals_intern.h

6.)
Congratulations, you are done.



Useful tools:
-------------
- emacs (add '(file-coding-system-alist (quote (("\\.locale\\'" . utf-8-unix) ("" undecided)))) to .emacs or use "C-x <RET> c utf-8 <RET> C-x C-f deutsch.locale")
- iconv
- sort
- uxterm
