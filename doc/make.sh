set -e
set -u


xmllint --noout --noent --valid manual.xml
xmlto -o manual html manual.xml
