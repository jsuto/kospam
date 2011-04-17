<html>
<head>
   <title>Napi karant�n riport <?php print $user['email']; ?> sz�m�ra</title>

   <meta http-equiv="content-type" content="text/html; charset=utf-8" />
   <meta http-equiv="Content-Language" content="<?php print LANG; ?>" />

   <link rel="stylesheet" type="text/css" href="<?php print SITE_URL; ?>view/theme/default/stylesheet/style.css" />
</head>
<body>

<h1>Napi karant�n riport <?php print $user['email']; ?> sz�m�ra</h1>

<p>�nnek <?php print $n; ?> db levele (<?php print $total_size; ?> byte) van a karant�nban a jelenlegi email h�zirendnek megfelel�en.</p>

<p>K�rj�k, hogy rendszeresen n�zze �t a karant�nt, mert ott csak korl�tozott ideig ker�lnek meg�rz�sre a levelek, azut�n automatikusan t�rl�dnek.</p>

<p>Figyelem: Bizonyos levelek t�rgysora aggressz�v �zenetet tartalmazhat!</p>

<p><a href="<?php print SITE_URL . "/index.php?route=quarantine/quarantine&amp;user=" . $user['username'] . "&amp;hamspam=SPAM"; ?>">Ide kattintva menedzselheti a karant�nt</a></p>


<table border="1">
<tr><td></td><td><strong>D�tum</strong></td><td><strong>M�ret</strong></td><td><strong>Felad�</strong></td><td><strong>T�rgy</strong></td></tr>

<?php foreach ($messages as $message) { ?>
<tr><td><a href="<?php print SITE_URL . "/index.php?route=quarantine/message&amp;id=" . $message['id'] . "&amp;user=" . $user['username'] . "&amp;page=0&amp;from=&amp;subj=&amp;hamspam=SPAM"; ?>"><?php print $message['i']; ?></a></td><td><?php print $message['date']; ?></td><td align="right"><?php print $message['size']; ?></td><td><?php print $message['shortfrom']; ?></td><td><?php print $message['shortsubject']; ?></td></tr>
<?php } ?>

</table>

</body>
</html>
