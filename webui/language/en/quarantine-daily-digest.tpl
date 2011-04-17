<html>
<head>
   <title>Quarantine digest for <?php print $user['email']; ?></title>

   <meta http-equiv="content-type" content="text/html; charset=utf-8" />
   <meta http-equiv="Content-Language" content="<?php print LANG; ?>" />

   <link rel="stylesheet" type="text/css" href="<?php print SITE_URL; ?>view/theme/default/stylesheet/style.css" />
</head>
<body>

<h1>Quarantine digest for <?php print $user['email']; ?></h1>

<p>You have <?php print $n; ?> email(s) (<?php print $total_size; ?> bytes) withheld from your mailbox in accordance with current email policy.</p>

<p>Please manage your messages regularlay as they are held for a limited time after which they are automatically deleted.</p>

<p>Note: Some subject lines may contain profane or offensive content.</p>

<p><a href="<?php print SITE_URL . "/index.php?route=quarantine/quarantine&amp;user=" . $user['username'] . "&amp;hamspam=SPAM"; ?>">Click here to manage your quarantined messages</a></p>


<table="1">
<tr><td></td><td><strong>Date</strong></td><td><strong>Size</strong></td><td><strong>From</strong></td><td><strong>Subject</strong></td></tr>

<?php foreach ($messages as $message) { ?>
<tr><td><a href="<?php print SITE_URL . "/index.php?route=quarantine/message&amp;id=" . $message['id'] . "&amp;user=" . $user['username'] . "&amp;page=0&amp;from=&amp;subj=&amp;hamspam=SPAM"; ?>"><?php print $message['i']; ?></a></td><td><?php print $message['date']; ?></td><td align="right"><?php print $message['size']; ?></td><td><?php print $message['shortfrom']; ?></td><td><?php print $message['shortsubject']; ?></td></tr>
<?php } ?>

</table>

</body>
</html>
