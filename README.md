# VertretungsBoy
A discord bot for the [representation plan of the DBG-Metzingen Gymnasium][link_school].

[link_school]: https://dbg-metzingen.de/vertretungsplan/zwei_tage.htm

# How to use it
Just places it anywhere and run it with pyhton3.
It expects as first parameter the path to the database. If there is no database, it recreates it for you. 
For the second parameter it expects the discord bot authentication token.

If your setup is correct, you should now see on discord that the bot is online. 
Just type ">>" in a chat, where the bot is allowed to answer, to see if it works.

# Dependencies
It requires the python libraries `pytz`, `requests`, `codecs`, `html`, `pysqlcipher3` and `datetime`. As `pysqlcipher3` is not longer available via pip, you should check out their [project page on GitHub][link_pysqlcipher3_github].

[link_pysqlcipher3_github]: https://github.com/rigglemania/pysqlcipher3
