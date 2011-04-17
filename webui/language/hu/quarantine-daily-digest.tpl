<html>
<head>
   <title>Napi karantén riport <?php print $user['email']; ?> számára</title>

   <meta http-equiv="content-type" content="text/html; charset=utf-8" />
   <meta http-equiv="Content-Language" content="<?php print LANG; ?>" />

   <link rel="stylesheet" type="text/css" href="<?php print SITE_URL; ?>view/theme/default/stylesheet/style.css" />
</head>
<body>

<h1>Napi karantén riport <?php print $user['email']; ?> számára</h1>

<p>Önnek <?php print $n; ?> db levele (<?php print $total_size; ?> byte) van a karanténban a jelenlegi email házirendnek megfelelően.</p>

<p>Kérjük, hogy rendszeresen nézze át a karantént, mert ott csak korlátozott ideig kerülnek megőrzésre a levelek, azután automatikusan törlődnek.</p>

<p>Figyelem: Bizonyos levelek tárgysora aggresszív üzenetet tartalmazhat!</p>

<p><a href="<?php print SITE_URL . "/index.php?route=quarantine/quarantine&amp;user=" . $user['username'] . "&amp;hamspam=SPAM"; ?>">Ide kattintva menedzselheti a karantént</a></p>


<table border="1">
<tr><td></td><td><strong>Dátum</strong></td><td><strong>Méret</strong></td><td><strong>Feladó</strong></td><td><strong>Tárgy</strong></td></tr>

<?php foreach ($messages as $message) { ?>
<tr><td><a href="<?php print SITE_URL . "/index.php?route=quarantine/message&amp;id=" . $message['id'] . "&amp;user=" . $user['username'] . "&amp;page=0&amp;from=&amp;subj=&amp;hamspam=SPAM"; ?>"><?php print $message['i']; ?></a></td><td><?php print $message['date']; ?></td><td align="right"><?php print $message['size']; ?></td><td><?php print $message['shortfrom']; ?></td><td><?php print $message['shortsubject']; ?></td></tr>
<?php } ?>

</table>

</body>
</html>
