from asyncio import Lock

import pytz
import requests
import codecs
from html.parser import HTMLParser
from pysqlcipher3 import dbapi2 as sqlCipher
import datetime

KEY = ""


class PlanError(Exception):
    pass


def prepare_grade(grade):
    if grade == "Früh":
        return " Frühvertretung "

    if grade == "\xa0":
        return grade

    grade_prefix = grade[0]

    result = ""

    for i, token in enumerate(grade[1:]):
        if token.isdigit() and grade_prefix[0].isdigit():
            if grade[i].isdigit() and grade_prefix[0].isdigit:
                grade_prefix += token
            else:
                grade_prefix = token
        else:
            result += grade_prefix + token + ", "

    return " " + result[:-2] + " "


def prepare_kind(kind):
    if kind == "Vertr.":
        return "Vertretung"
    elif kind == "Vtr. o. Leh.":
        return "Vertretung ohne Lehrer"
    elif kind == "Raumänd.":
        return "Raumänderung"
    elif kind == "Sondereins.":
        return "Sondereinsatz"

    return kind


def prepare_subject(old_subject, new_subject):
    if new_subject == "Früh" and old_subject == "Früh":
        return '\xa0'
    elif new_subject == "---" or new_subject == '\xa0' or new_subject == old_subject:
        return " " + old_subject + " "
    elif old_subject == "---" or old_subject == '\xa0':
        return " " + new_subject + " "
    else:
        return " " + new_subject + " statt " + old_subject + " "


def prepare_room(room):
    if room == "---":
        return '\xa0'
    else:
        return room


def quote_identifier(s, errors="strict"):
    encodable = s.encode("utf-8", errors).decode("utf-8")

    nul_index = encodable.find("\x00")

    if nul_index >= 0:
        error = UnicodeEncodeError("NUL-terminated utf-8", encodable,
                                   nul_index, nul_index + 1, "NUL not allowed")
        error_handler = codecs.lookup_error(errors)
        replacement, _ = error_handler(error)
        encodable = encodable.replace("\x00", replacement)

    return "\"" + encodable.replace("\"", "\"\"") + "\""


class Plan(HTMLParser):
    def __init__(self, db_path, urls, auto_update_times):
        self.db_path = db_path
        self.auto_update_times = auto_update_times
        self.urls = urls
        self.last_attr = []
        self.row = []
        self.current_url = ""
        self.conn = None
        self.last_prepared_row = []
        self.db_mutex = Lock()
        super().__init__()

    async def get_database(self):
        await self.db_mutex.acquire()
        self.conn = sqlCipher.connect(self.db_path)
        self.conn.execute('pragma key="' + KEY + '"')
        return self.conn

    def close_database(self):
        self.conn.close()
        self.db_mutex.release()

    @staticmethod
    def localize_time(time):
        return pytz.timezone('Europe/Berlin').localize(time)

    async def run_database_operation(self, run, *args):
        self.close_database()
        try:
            return await run(*args)
        finally:
            await self.get_database()

    async def update(self):
        try:
            await self.get_database()

            self.conn.execute("CREATE TABLE IF NOT EXISTS dates (side TEXT UNIQUE, date TEXT)")
            self.conn.execute("CREATE TABLE IF NOT EXISTS info (side TEXT, info TEXT)")

            for url in self.urls:
                website = requests.get(url)
                self.current_url = url

                self.conn.execute("DELETE FROM dates WHERE side == ?", (url,))
                self.conn.execute("DELETE FROM info WHERE side == ?", (url,))

                self.conn.execute("DROP TABLE IF EXISTS " + quote_identifier(url))
                self.conn.execute(
                    """CREATE TABLE """ + quote_identifier(url) + """ (
                            grade TEXT,
                            lessons TEXT,
                            kind TEXT, 
                            subjects TEXT, 
                            rooms TEXT, 
                            text TEXT
                    );"""
                )

                self.feed(codecs.decode(website.content, website.encoding))

            # noinspection SqlResolve
            self.conn.execute("REPLACE INTO dates (side, date) VALUES (\"intern\", ?)", (int(datetime.datetime.now().timestamp()),))
        finally:
            self.conn.commit()
            self.close_database()

            self.last_attr = []
            self.row = []
            self.current_url = ""
            self.last_prepared_row = []

    def handle_starttag(self, tag, attrs):
        if tag != "span":
            self.last_attr = attrs

    def handle_endtag(self, tag):
        if tag == "tr" or tag == "div":
            if len(self.last_attr) > 0 and self.last_attr[0][0] == "class" and len(self.row) > 0:
                if self.last_attr[0][1] == "info":
                    # region insert info on side into table
                    info = ""

                    for part in self.row:
                        info += part

                    self.conn.execute("INSERT INTO info (side, info) VALUES (?, ?);", (self.current_url, info))
                    # endregion

                elif self.last_attr[0][1] == "mon_title":
                    # region insert date of plan into table
                    self.conn.execute("INSERT INTO dates (side, date) VALUES (?, ?)", (self.current_url, self.row[0]))
                    # endregion

                elif self.last_attr[0][1] == "list" and len(self.row) == 7:
                    prepared_row = [
                        prepare_grade(self.row[0]),
                        self.row[1],
                        prepare_kind(self.row[2]),
                        prepare_subject(self.row[3], self.row[4]),
                        prepare_room(self.row[5]),
                        self.row[6]
                    ]

                    if prepared_row[:5] == ['\xa0', '\xa0', '\xa0', '\xa0', '\xa0']:
                        # region completes text at the previous row in the table
                        self.conn.execute(
                            """
                                UPDATE """ + quote_identifier(self.current_url) + """
                                SET text = ?
                                WHERE 
                                grade == ? and lessons == ? and kind == ? and subjects == ? and rooms == ? and text == ?
                            """,
                            (
                                self.last_prepared_row[5] + " " + prepared_row[5],
                                self.last_prepared_row[0],
                                self.last_prepared_row[1],
                                self.last_prepared_row[2],
                                self.last_prepared_row[3],
                                self.last_prepared_row[4],
                                self.last_prepared_row[5]
                            )
                        )
                        # endregion
                    else:
                        # region insert new row into table
                        if prepared_row[0] == '\xa0':
                            prepared_row[0] = self.last_prepared_row[0]

                        if prepared_row[0] == "AG":
                            if prepared_row[3] == "Klettern":
                                prepared_row[3] = "Kletter"

                            prepared_row[0] = prepared_row[3] + " AG"
                            prepared_row[3] = '\xa0'

                        self.conn.execute(
                            """
                                  INSERT INTO """ + quote_identifier(self.current_url) + """
                                  (grade, lessons, kind, subjects, rooms, text) VALUES (?, ?, ?, ?, ?, ?)
                            """,
                            prepared_row
                        )
                        # endregion

                    self.last_prepared_row = prepared_row

                self.row = []

    def handle_data(self, data):
        if len(self.last_attr) > 0 and self.last_attr[0][0] == "class" and self.lasttag != "th":
            if self.last_attr[0][1] == "list" or \
               self.last_attr[0][1] == "info" or \
               self.last_attr[0][1] == "mon_title":
                if data != '\r\n':
                    self.row.append(data)

    async def search(self, user_id, search):
        result = {}

        try:
            await self.get_database()

            self.conn.execute("CREATE TABLE IF NOT EXISTS searches (user_id TEXT UNIQUE, search TEXT)")

            if user_id is not None:
                if search is None or len(search) < 1:
                    search = await self.run_database_operation(self.get_last_user_search, user_id)
                else:
                    # region safe search from into the table
                    self.conn.execute("REPLACE INTO searches (user_id, search) VALUES (?, ?)", (user_id, search))
                    self.conn.commit()
                    # endregion

                search = search.split()
                search[0] = "%" + search[0] + "%"

                for i in range(1, len(search)):
                    search[i] = "% " + search[i] + " %"

            # region check for updates
            update_date = await self.run_database_operation(self.get_update_date)
            update_date = datetime.datetime.utcfromtimestamp(update_date)
            update_date = pytz.utc.localize(update_date)

            for auto_update_time in self.auto_update_times:
                auto_update_time = datetime.datetime.strptime(
                    str(datetime.datetime.now().day) + "." +
                    str(datetime.datetime.now().month) + "." +
                    str(datetime.datetime.now().year) + " " +
                    auto_update_time,
                    "%d.%m.%Y %H:%M"
                )

                auto_update_time = self.localize_time(auto_update_time)
                now = pytz.utc.localize(datetime.datetime.now())

                if update_date < auto_update_time < now:
                    await self.run_database_operation(self.update)
                    update_date = datetime.datetime.now()
                    update_date = pytz.utc.localize(update_date)

            # endregion

            self.conn.execute("CREATE TABLE IF NOT EXISTS dates (side TEXT UNIQUE, date TEXT)")
            self.conn.execute("CREATE TABLE IF NOT EXISTS info (side TEXT, info TEXT)")

            for url in self.urls:
                # region get plan date
                plan_date = self.conn.execute("SELECT date FROM dates WHERE side == ?", (url,))
                plan_date = plan_date.fetchone()

                if plan_date is None or len(plan_date) < 1:
                    raise PlanError("missing plan date for " + quote_identifier(url))

                plan_date = plan_date[0]

                if datetime.datetime(  # Today at 0:00
                        datetime.datetime.now().year,
                        datetime.datetime.now().month,
                        datetime.datetime.now().day
                ) > datetime.datetime.strptime(plan_date[:plan_date.find(" ")], "%d.%m.%Y"):  # Date of plan at 0:00
                    continue

                # endregion

                # region get info
                info = self.conn.execute("SELECT info FROM info WHERE side == ?", (url,))
                info = info.fetchall()
                # endregion

                # region get entries
                if user_id is not None:
                    sql_query = "SELECT * FROM " + quote_identifier(url) + " WHERE grade LIKE ? AND ("

                    if len(search) > 1:
                        sql_query += "subjects LIKE ? OR " * (len(search) - 1)
                        sql_query = sql_query[:-3] + ")"
                    else:
                        sql_query = sql_query[:-5]

                    entries = self.conn.execute(sql_query, search)
                    entries = entries.fetchall()
                else:
                    entries = []
                # endregion

                result[plan_date] = (info, entries)

        finally:
            self.close_database()

        return result

    async def get_update_date(self):
        try:
            await self.get_database()
            self.conn.execute("CREATE TABLE IF NOT EXISTS dates (side TEXT UNIQUE, date TEXT)")
            # noinspection SqlResolve
            cursor = self.conn.execute("SELECT date FROM dates WHERE side == \"intern\"")

            update_date = cursor.fetchone()
            if update_date is None or len(update_date) < 1:
                await self.run_database_operation(self.update)
                update_date = (datetime.datetime.now().timestamp(),)
        finally:
            self.close_database()

        return int(update_date[0])

    async def get_last_user_search(self, user_id):
        try:
            await self.get_database()

            self.conn.execute("CREATE TABLE IF NOT EXISTS searches (user_id TEXT UNIQUE, search TEXT)")
            search = self.conn.cursor()
            search.execute("SELECT search FROM searches WHERE user_id == ?", (user_id,))

            search = search.fetchone()
            if search is None or len(search) < 1:
                raise PlanError("no last search found")
        finally:
            self.close_database()

        return search[0]

# example
#
# plan = Plan(
#     "plan.db",
#     [
#         "https://dbg-metzingen.de/vertretungsplan/tage/subst_001.htm",
#         "https://dbg-metzingen.de/vertretungsplan/tage/subst_002.htm"
#     ],
#     [
#         "8:30",
#         "15:00"
#     ]
# )
# print(plan.search(None, ""))
# print(plan.search(23424, "10a"))
# print(plan.search(23424, "K2 L ev2"))
# print(datetime.datetime.fromtimestamp(plan.get_update_date()))
