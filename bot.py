import sys

import discord
import pytz

from plan import Plan
import datetime

import sys
if len(sys.argv) < 3:
    sys.exit("no database path or token applied")

client = discord.Client()
plan = Plan(
    sys.argv[1],
    [
        "https://dbg-metzingen.de/vertretungsplan/tage/subst_001.htm",
        "https://dbg-metzingen.de/vertretungsplan/tage/subst_002.htm"
    ],
    ["8:30", "15:00"]
)


async def plan_error_catcher(message, run):
    try:
        await run(message)
    except Exception as error:
        error_msg = "Ooops, das hat wohl nicht funktioniert :no_mouth:\n\n" + \
                    "`" + error.args[0] + "`"

        if error.args[0] == "no last search found":
            error_msg += "\n\n"
            error_msg += "**Wahrscheinlich hast du einfach noch nie nach etwas gesucht**.\n"
            error_msg += "Gib einfach nach den Befehl ein Leerzeichen und deine Suche ein, "
            error_msg += "dann musst du sie beim nächsten Mal nicht mehr eingeben :blush:\n"
            error_msg += "Wenn du noch mehr Hilfe brauchst, benutze `h` oder `help` um dir die Hilfe anzeigen zu lassen.\n"
            error_msg += "Falls du danach immer noch Probleme hast, melde dich einfach bei mir :relaxed:\n"

        await message.channel.send(
            error_msg
        )


async def plan_command_update(message):
    await plan.update()
    await message.channel.send(
        "Fertig, alles aktualisiert :blush:"
    )


async def plan_command_date(message):
    last_update = await plan.get_update_date()
    last_update = datetime.datetime.utcfromtimestamp(last_update)
    last_update = pytz.utc.localize(last_update, is_dst=None).astimezone(pytz.timezone('Europe/Berlin'))
    plan_dates = await plan.search(None, None)

    content = ""

    if len(plan_dates) < 1:
        content += "Kein Plan ist aktuelle... :confused:\n\n"

    for date, info in plan_dates.items():
        content += date + "\n"

    content += "\nZuletzt aktualisiert am " + last_update.strftime("%d.%m.%Y") + " um " + last_update.strftime("%H:%M")

    await message.channel.send(content)


async def add_to_content(message, content, add):
    if len(content) + len(add) > 2000:
        await message.channel.send(content)
        if content[-2:] == "\n\n":
            content = "​\n"  # with zero white space
        else:
            content = "​"

    return content + add


def string_empty(string):
    return string == '   ' or string == '\xa0'


async def plan_command_info(message):
    search = message.content[2:].lstrip()

    if search.find(" ") > 0:
        search = search[search.find(" "):]
    else:
        search = ""

    entries = await plan.search(message.author.id, search)

    content = ""

    if len(search) < 1:
        search = await plan.get_last_user_search(message.author.id)
        search = search.replace("%", "")

    for key in entries.keys():
        content = await add_to_content(message, content, "__**" + key[:key.find(" ")] + "**__\n\n")

        if len(entries[key][0]) > 0:
            content = await add_to_content(message, content, "Info zum Tag:\n")

            for info in entries[key][0]:
                content = await add_to_content(message, content, "- " + info[0] + "\n")

            content = await add_to_content(message, content, "\n")

        if len(entries[key][1]) > 0:
            for row in entries[key][1]:
                if not string_empty(row[0]):
                    if row[0] == "Frühvertretung" or row[0][-2:] == "AG":
                        content = await add_to_content(message, content, "**" + row[0] + "**\n")
                    elif row[0].find(",") > 0:
                        content = await add_to_content(message, content, "**Klassen:   " + row[0] + "**\n")
                    else:
                        content = await add_to_content(message, content, "**Klasse:      " + row[0] + "**\n")

                if not string_empty(row[1]):
                    if row[1].find("-") > 0:
                        content = await add_to_content(message, content, "Stunden:    " + row[1] + "\n")
                    else:
                        content = await add_to_content(message, content, "Stunde:      " + row[1] + "\n")

                if not string_empty(row[2]):
                    content = await add_to_content(message, content, "Art:             " + row[2] + "\n")

                if not string_empty(row[3]):
                    content = await add_to_content(message, content, "Fach:         " + row[3] + "\n")

                if not string_empty(row[4]):
                    content = await add_to_content(message, content, "Raum:        " + row[4] + "\n")

                if not string_empty(row[5]):
                    content = await add_to_content(message, content, "Text:          " + row[5] + "\n")

                content = await add_to_content(message, content, "\n")

        else:
            content = await add_to_content(message, content, "Nope, nichts da für `" + search + "` :neutral_face:\n\n")

    await message.channel.send(content)


def replace_last_comma(string):
    pos = string.rfind(",")
    return string[:pos] + " und " + string[pos + 1:]


async def plan_command_subscribe(message):
    times = message.content[2:].lstrip()
    times = times.split()
    times = times[1:]

    try:
        db = await plan.get_database()
        db.execute("CREATE TABLE IF NOT EXISTS subscriptions (user INTEGER, time INTEGER, last INTEGER)")

        if len(times) == 0:
            db.execute("DELETE FROM subscriptions WHERE user == " + str(message.author.id) + ";")
            db.commit()
            await message.channel.send("Okay, keine Benachrichtigungen mehr für dich :slight_smile:")

        else:
            # region check if last search is available
            plan.close_database()
            try:
                last_search = await plan.get_last_user_search(message.author.id)
            finally:
                db = await plan.get_database()
            # endregion

            # region get valid times
            invalid_times = []
            valid_times = []

            for time in times:
                try:
                    time = plan.localize_time(datetime.datetime.strptime(time, "%H:%M"))
                    valid_times.append(time)
                except Exception as error:
                    if error.args[0] == "time data '" + time + "' does not match format '%H:%M'"\
                            or error.args[0][:24] == "unconverted data remains":
                        invalid_times.append(time)
                    else:
                        raise error

            # endregion

            if len(valid_times) == 0:
                await message.channel.send("Du hast keine korrekte Zeit angegeben. Deswegen wird nichts an deinem Abo verändert")
            else:
                # region save to times to database
                db.execute("DELETE FROM subscriptions WHERE user == " + str(message.author.id) + ";")

                for valid_time in valid_times:
                    db.execute("INSERT INTO subscriptions (user, time) VALUES (" + str(message.author.id) + ", " + str(valid_time.hour * 100 + valid_time.minute) + ");")

                db.commit()
                # endregion

                # region generate confirmation message
                valid_times.sort()

                subscription_active_notification = "Okay, bekommst jetzt von Sonntag bis Montag " + \
                    "immer eine Benachrichtung zur deiner aktuellen letzte Suche. Die ist übrigens gerade `" + last_search + \
                    "`.\nDu hast dir "

                for valid_time in valid_times:
                    subscription_active_notification += "`" + valid_time.astimezone(pytz.timezone('Europe/Berlin')).strftime("%H:%M") + "`, "

                subscription_active_notification = subscription_active_notification[:-2] + " als "

                if len(valid_times) == 1:
                    subscription_active_notification += "Uhrzeit "
                else:
                    subscription_active_notification = replace_last_comma(subscription_active_notification)
                    subscription_active_notification += "Uhrzeiten "

                subscription_active_notification += "ausgesucht.\n"

                if len(invalid_times) > 0:
                    if len(invalid_times) == 1:
                        subscription_active_notification += "Der Parameter `" + invalid_times[0] + "` wurde ignoriert, " +\
                            "da es keine Uhrzeit ist."
                    else:
                        subscription_active_notification += "Die Parameter "

                        for invalid_time in invalid_times:
                            subscription_active_notification += "`" + invalid_time + "`, "

                        subscription_active_notification = subscription_active_notification[:-2]
                        subscription_active_notification = replace_last_comma(subscription_active_notification)
                        subscription_active_notification += " wurden ignoriert, da es keine Uhrzeiten sind."

                await message.channel.send(subscription_active_notification)
                # endregion

    except Exception as error:
        # region handel no last search available
        if error.args[0] == "no last search found":
            await message.channel.send(
                "Du musst mindestens einmal was gesucht haben, da du mit den Abo deine letzte Suche abonnierst. " +
                "Also benutzt noch kurz den `i` oder `info` Befehl.\n" +
                "Wenn du noch mehr Hilfe brauchst, benutze `h` oder `help` um dir die Hilfe anzeigen zu lassen.\n" +
                "Falls du danach immer noch Probleme hast, melde dich einfach bei mir :relaxed:\n"
            )
        # endregion
        else:
            raise error
    finally:
        plan.close_database()


@client.event
async def on_message(message):
    # we do not want the bot to reply to itself
    if message.author == client.user:
        return

    if message.content.startswith(">>"):
        command = message.content[2:].lower()
        command = command.lstrip()

        if command.find(" ") > 0:
            command = command[:command.find(" ")]

        if len(command) < 1:
            await message.channel.send(
                "Brauchst du Hilfe?\n" +
                "Dann benutze den Befehle `h` oder `help`, um dir die Hilfe anzeigen zu lassen :blush:"
            )

        elif command == "h" or command == "help":
            await message.channel.send(
                "__**Befehle**__:\n\n" +
                "Befehle werden durch `>>` am Anfang gekennzeichnet, " +
                "alle weitere Parameter werden durch Leerzeichen getrennt. " +
                "Groß- und Kleinschreibung wird nicht beachtet.\n\n" +
                "`i` oder `info`\n" +
                "Zeigt die Einträge in der Datenbank, die deiner Suche entsprechen und die Info zum Tag an. " +
                "Als Paramter kann zuerst eine Klasse angegeben werden und " +
                "danach all die Fächer, nach denen ausschließlich gesucht werden soll. " +
                "Falls nichts angegeben wird, wird die letzte Suchanfrage des jeweiligen Benutzters vewenden, " +
                "falls es möglich ist.\n\n" +
                "`s` oder `subscribe`\n" +
                "Abonniert die letzte Suche des jeweiligen Benutzters, sodass sie ihm jeden Tag außer Samstag " +
                "als private Nachricht gesendet wird. " +
                "Als Parameter werden alle Uhrzeiten, an denen man die Nachrichten bekommen will, angegeben. " +
                "Ein Uhrzeiten muss mit Stunde und Minute angegeben werden, die von einem Doppeltpunkt getrennt werden. " +
                "Zwischen mehreren Uhrzeiten müssen Leerzeichen.\n" +
                "Wenn kein Parameter angegeben wird, deabonniert man.\n\n"
                "`u` oder `update`\n" +
                "Aktualisiert die Datenbank. " +
                "Sollte nur bei Fehlern in der Datenbank aufgerufen werden, " +
                "da sie sich ansonst automatisch aktualisiert.\n\n" +
                "`d` oder `date`\n" +
                "Zeigt die Datums zu den Plänen in der Datenbank an " +
                "und wann die Datenbank zuletzt aktualisiert worden ist.\n\n" +
                "`h` oder `help`\n" +
                "Zeigt diese Hilfe, offensichtlich :joy:"
            )
        elif command == "u" or command == "update":
            await plan_error_catcher(
                message,
                plan_command_update
            )

        elif command == "d" or command == "date":
            await plan_error_catcher(
                message,
                plan_command_date
            )
        elif command == "i" or command == "info":
            await plan_error_catcher(
                message,
                plan_command_info
            )
        elif command == "s" or command == "subscribe":
            await plan_error_catcher(
                message,
                plan_command_subscribe
            )
        else:
            await message.channel.send(
                "Neee, `" + command + "` ist definitiv kein Befehl... :thinking:\n" +
                "Versuch mal `h` oder `help`, dann wird dir die Hilfe angezeigt :relaxed:"
            )


# noinspection PyUnusedLocal
@client.event
async def on_message_edit(before, after):
    await on_message(after)


@client.event
async def on_ready():
    print(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
    print("Logged in as")
    print(client.user.name)
    print(client.user.id)
    print("------")

client.run(str(sys.argv[2]))
