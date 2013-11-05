
# This script converts all the mkd files to html files.
#
# The file "template.html" was generated by running "wpgen.py init" (only once).
# We have modified the "template.html" file based on the style
# we want, so we should NOT run ""wpgen.py init" again.
#
# The wpgen commands will produce HTML pages in the "html" folder.
#
# The first time we run "wpgen.py init" on Mac, we may encounter the following error:
#
#  IOError: [Errno 2] No such file or directory: '/usr/share/wpgen/template_Default.html'
#
# If so, copy the files under "/usr/share/wpgen" on a linux machine (e.g., calvin.calit2.uci.edu) 
# to the folder /usr/share/wpgen.  Then the problem will be solved.

echo wpgen.py updateall
wpgen.py updateall

# replace the "title" tag with the correct value

echo "Replacing the HTML TITLE value from html/*.html files and producing new HTML files in the currnet folder"
if [ -f html/main.html ]; then sed -e 's#<title>.*</title>#<title>SRCH2: Manual</title>#g' html/main.html > main.html  ; fi
if [ -f html/install.html ]; then sed -e 's#<title>.*</title>#<title>SRCH2: Installation</title>#g' html/install.html > install.html  ; fi
if [ -f html/configuration.html ]; then sed -e 's#<title>.*</title>#<title>SRCH2: Configuration</title>#g' html/configuration.html > configuration.html  ; fi
if [ -f html/restful-search.html ]; then sed -e 's#<title>.*</title>#<title>SRCH2: Search API</title>#g' html/restful-search.html > restful-search.html  ; fi
if [ -f html/restful-insert-update-delete.html ]; then sed -e 's#<title>.*</title>#<title>SRCH2: Insert/Update/Delete API</title>#g' html/restful-insert-update-delete.html > restful-insert-update-delete.html  ; fi
if [ -f html/restful-control.html ]; then sed -e 's#<title>.*</title>#<title>SRCH2: Control API</title>#g' html/restful-control.html > restful-control.html  ; fi
if [ -f html/ranking.html ]; then sed -e 's#<title>.*</title>#<title>SRCH2: Ranking</title>#g' html/ranking.html > ranking.html  ; fi
if [ -f html/geo.html ]; then sed -e 's#<title>.*</title>#<title>SRCH2: Geo Search</title>#g' html/geo.html > geo.html  ; fi
if [ -f html/mongodb.html ]; then sed -e 's#<title>.*</title>#<title>SRCH2: MongoDB Search</title>#g' html/mongodb.html > mongodb.html  ; fi
