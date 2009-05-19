<?php

include_once("config.php");

session_start();

include_once("header.php");

logout();

?>


  <h3><?php print $LOGOUT; ?></h3>

  <div id="body">

<p>

<?php print $err_you_are_logged_out; ?>

</p>


      </td>
      <td>
      </td>
    </tr>
   </table>


  </div> <!-- body -->




<?php include_once("footer.php"); ?>
