#!/bin/sh

echo 'module.exports = {' > dataset.js
find dataset -type f | while read f; do
  echo '  "'"$f"'":"'`nodejs print.js "$f"`'",' >> dataset.js
done
echo '};' >> dataset.js
