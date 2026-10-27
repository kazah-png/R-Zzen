from pathlib import Path
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak
from reportlab.lib import colors
from reportlab.lib.styles import ParagraphStyle
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont

ROOT = Path(__file__).parent
OUT = ROOT / 'output' / 'pdf'
OUT.mkdir(parents=True, exist_ok=True)
pdfmetrics.registerFont(TTFont('Segoe', 'C:/Windows/Fonts/segoeui.ttf'))
pdfmetrics.registerFont(TTFont('SegoeBold', 'C:/Windows/Fonts/segoeuib.ttf'))
pdfmetrics.registerFontFamily('Segoe', normal='Segoe', bold='SegoeBold')
ink, teal, muted = [colors.HexColor(x) for x in ('#162332', '#007b78', '#526275')]
styles = {n: ParagraphStyle(n, fontName=f, fontSize=s, leading=l, textColor=c, spaceAfter=a)
          for n, f, s, l, c, a in [
              ('body', 'Segoe', 10, 15, ink, 9), ('title', 'SegoeBold', 36, 42, ink, 14),
              ('head', 'SegoeBold', 21, 27, ink, 15), ('sub', 'SegoeBold', 12, 18, teal, 8),
              ('small', 'Segoe', 9, 13, muted, 8)]}
story = []
def p(text, style='body'): return Paragraph(text, styles[style])
def add(text, style='body'): story.append(p(text, style))
def table(rows):
    t = Table([[p(a, 'small'), p(b, 'small')] for a, b in rows], colWidths=[140, 367])
    t.setStyle(TableStyle([
        ('VALIGN', (0, 0), (-1, -1), 'TOP'),
        ('BACKGROUND', (0, 0), (0, -1), colors.HexColor('#edf5f4')),
        ('LINEBELOW', (0, 0), (-1, -1), .5, colors.HexColor('#d9e2e7')),
        ('LEFTPADDING', (0, 0), (-1, -1), 10), ('RIGHTPADDING', (0, 0), (-1, -1), 10),
        ('TOPPADDING', (0, 0), (-1, -1), 10), ('BOTTOMPADDING', (0, 0), (-1, -1), 8)]))
    story.append(t)
def page(number, title):
    story.append(PageBreak())
    add(f'USER GUIDE / {number:02d}', 'small')
    add(title, 'head')

add('VISUAL GAME AUTOMATION', 'sub')
add('Zzen', 'title')
add('User guide', 'head')
add('v0.2 / Native C++ / Windows x64<br/>Roblox - Army Commander Tycoon', 'small')
story.append(Spacer(1, 12))
add('One session. The complete cycle.', 'sub')
add('Zzen keeps your character active, buys available upgrades and attempts promotion when the requirements are met. After confirming a promotion, it returns to buying upgrades. These features work together through one Start session button.')
add('Quick start', 'sub')
for title, body in [
    ('1. Open Zzen', 'Extract the complete package, close any older copy and launch <b>Zzen.exe</b>.'),
    ('2. Prepare the game', 'Join the tycoon and set the <b>game language to Spanish</b> using page 2. Keep the bottom toolbar visible and close chat and unrelated menus. The tested resolution is 1920 x 1080.'),
    ('3. Start the session', 'Press <b>F8</b> or click <b>Start session</b>. Zzen minimizes and brings Roblox forward. Leave the controls untouched for two seconds.'),
    ('4. Follow the status', 'A small translucent strip at the top centre shows what Zzen is doing. It lets clicks pass through and disappears when the session stops or you switch applications.'),
    ('5. Stop whenever needed', 'Press <b>F8</b> to stop; press it again to start, even while Zzen is minimized. Stop or closing Zzen also ends the session. Switching apps pauses inputs until you return.')]:
    add(title, 'sub'); add(body)
add('<b>Promotion resets the base and in-game money.</b> Starting a session includes automatic promotion. Stop Zzen before taking manual control or running another autoclicker.', 'small')

page(2, 'Set the game to Spanish')
add('Required for recognition', 'sub')
add('The Zzen interface is in English, but its current game profile reads Spanish menu labels. Leave Zzen in English and set the Roblox experience to Spanish before starting automation.')
add('Option A - Change this game only', 'sub')
add('1. Join the experience and open the Roblox in-game menu with <b>Esc</b>.<br/>2. Open <b>Settings</b> and look for <b>Game Language</b> (also called <b>Experience Language</b>).<br/>3. Select <b>Spanish</b> or <b>Español</b>.<br/>4. Return to the game. If the text has not changed, leave and rejoin before starting Zzen.')
add('Option B - Change your Roblox account language', 'sub')
add('If the game language option is unavailable, open Roblox in your browser and sign in. Go to <b>Settings &gt; Account Info &gt; Personal &gt; Language</b>, choose <b>Spanish / Español</b> and click <b>Save</b> if the page does not reload automatically. Rejoin the experience and check its menus.')
add('Check the result before starting', 'sub')
table([
    ('Build toolbar', 'The build control should read <b>CONSTRUIR</b>.'),
    ('Construction panel', 'Press B. The panel should show <b>CONSTRUCCIÓN</b>, with upgrade actions such as <b>Mejora</b>.'),
    ('More menu', 'The three-dot menu is labelled <b>MÁS</b>. Its promotion entry should read <b>PROMOCIONAR</b>.'),
    ('Still in English?', 'Stop Zzen and check the game language again. Some experience text may not be translated by Roblox. Confirm the visible labels before proceeding.')])
story.append(Spacer(1, 12))
add('After checking, close the panels with their X buttons and leave the bottom toolbar visible. You do not need to change the Windows display language.', 'small')
add('References: <link href="https://en.help.roblox.com/hc/en-us/articles/40269095851284-Enabling-Automatic-Translations-in-Account-Settings" color="#007b78">Roblox Support - game language</link>; <link href="https://en.help.roblox.com/hc/en-us/articles/360001216486-Changing-Your-Account-Language" color="#007b78">Roblox Support - account language</link>.', 'small')

page(3, 'How the session works')
table([
    ('Buy upgrades', 'Zzen detects the purchase notification, presses B and uses the selection chosen by the game. It requires a recognised upgrade action, price and yellow button.'),
    ('Verify each upgrade', 'It looks for a level increase. A menu closing by itself is not counted as a verified upgrade. Zzen uses the X button when the panel remains open.'),
    ('Check promotion', 'It locates the current three-dot icon, opens More and selects the promotion entry. It reads requirements, cost and promotion count. A grey button is not treated as available.'),
    ('Save for promotion', 'When the citadel is recognised as complete, Zzen stops buying upgrades to save money. It checks promotion availability approximately once per minute.'),
    ('Attempt promotion', 'Zzen checks availability twice and saves the previous count. It then recognises the reset confirmation, checks it twice and confirms once. The pending record survives an app restart.'),
    ('Confirm and continue', 'It must read the previous count plus one twice. It then clears the pending record and returns to the purchase cycle.'),
    ('Needs attention', 'An uncertain promotion suspends purchases and promotion; it is never repeated blindly. Keep-active remains eligible only on a clear game view. Keep the pending record and export the log.')])
story.append(Spacer(1, 14))
add('Keep-active behaviour', 'sub')
add('The anti-AFK action sends a jump approximately every three minutes when the game view is recognised. Keep-active actions, purchases and promotion belong to the same session; there are no separate mode switches.')
add('Operating conditions', 'sub')
add('Keep Roblox visible and in the foreground, with the computer awake and unlocked. Zzen pauses inputs when another application is active. It does not reconnect a disconnected game, manage combat or units, or automate premium purchases.')
add('Zzen requests that Windows stay awake during an active session. It rechecks menu targets before clicking and retries temporary reading failures. Avoid using game controls during automation.', 'small')

page(4, 'Troubleshooting and feedback')
table([
    ('Nothing happens', 'Read the status. Check that the session is started, Roblox is in the foreground, the game is in Spanish and the bottom toolbar is visible. Leave the controls untouched for a few seconds.'),
    ('Saving for promotion', 'This is expected after the citadel is complete. Compare your balance with the promotion cost. Zzen intentionally pauses upgrade purchases while saving.'),
    ('More is not found', 'Day and night layouts can move the toolbar. Keep the full toolbar visible. Unclear text can be read again for up to 30 seconds without using click attempts. Navigation failures trigger automatic recovery: close a recognised menu, wait and retry.'),
    ('A panel stays open', 'Press F8 and inspect the panel. Close it manually with its X, then return to a clear game view before starting again.'),
    ('Promotion is uncertain', 'Stop the session and check the promotion count manually. Keep promocion-pendiente.txt if present. On restart, Zzen uses it to verify the previous attempt before buying again.')])
story.append(Spacer(1, 12))
add('Share an error in a few clicks', 'sub')
add('1. Stop the session with <b>F8</b> when something goes wrong.<br/>2. Click <b>Export log</b> in Zzen.<br/>3. The <b>feedback</b> folder opens with a dated <b>Zzen-feedback-...txt</b> file.<br/>4. Attach that text file to the support chat and briefly describe what you expected and what happened.')
add('The export contains version, status, recent events, recovery counts and the last keep-active time. It contains no screenshots and is not sent automatically. The session log and local diagnostic images remain separate.', 'small')
add('Optional diagnostic', 'sub')
add('With automation stopped, click <b>Run diagnostic</b> and return to the problem screen during the five-second countdown. The diagnostic saves local evidence in <b>diagnostico-cpp</b>. Use Export log for the normal text-only feedback workflow.')
add('Validation status', 'sub')
add('Version 0.2 adds regression checks for the real BUILD $700 recognition failure, changed click targets, cancellation and a four-hour simulated recovery run. This simulation is separate from live testing. See <b>VALIDACION.txt</b> for the final results, observed run length and limits.', 'small')

def frame(canvas, doc):
    canvas.setFillColor(teal); canvas.rect(44, 812, 507, 3, fill=1, stroke=0)
    canvas.setStrokeColor(colors.HexColor('#d9e2e7')); canvas.line(44, 40, 551, 40)
    canvas.setFont('Segoe', 8); canvas.setFillColor(muted)
    canvas.drawString(44, 25, 'ZZEN v0.2 / USER GUIDE')
    canvas.drawRightString(551, 25, f'{doc.page} / 4')

destination = OUT / 'Zzen-v0.2-User-Guide.pdf'
doc = SimpleDocTemplate(str(destination), pagesize=(595, 842), leftMargin=44, rightMargin=44,
                        topMargin=49, bottomMargin=54, title='Zzen v0.2 - User Guide', author='Zzen')
doc.build(story, onFirstPage=frame, onLaterPages=frame)
print(destination)
