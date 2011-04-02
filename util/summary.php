<?php

$webuidir = "";

if(isset($_SERVER['argv'][1])) { $webuidir = $_SERVER['argv'][1]; }

require_once($webuidir . "/config.php");

require(DIR_SYSTEM . "/startup.php");


$loader = new Loader();
Registry::set('load', $loader);

$loader->load->helper('libchart/classes/libchart');
$loader->load->helper('fpdf16/fpdf');

$loader->load->model('stat/chart');

$language = new Language();
Registry::set('language', $language);


if(DB_DRIVER == "ldap"){
   $db = new LDAPDB(DB_DRIVER, LDAP_HOST, LDAP_BINDDN, LDAP_BINDPW);
}
else {
   $db = new DB(DB_DRIVER, DB_HOSTNAME, DB_USERNAME, DB_PASSWORD, DB_DATABASE, DB_PREFIX);
   Registry::set('DB_DATABASE', DB_DATABASE);
}

Registry::set('db', $db);

Registry::set('DB_DRIVER', DB_DRIVER);

Registry::set('HISTORY_DATABASE', HISTORY_DATABASE);
Registry::set('HISTORY_DRIVER', HISTORY_DRIVER);



$db_history = new DB(HISTORY_DRIVER, DB_HOSTNAME, DB_USERNAME, DB_PASSWORD, HISTORY_DATABASE, DB_PREFIX);
Registry::set('db_history', $db_history);


$aa = new ModelStatChart();

$timespan = "daily";
$title = "Ham/Spam messages";
$pdf_title = "";

/* Note: the php-cgi binary has the $_REQUEST array ... */

if(isset($_SERVER['argv'][2])) { $pdf_title = $_SERVER['argv'][2]; }
if(isset($_SERVER['argv'][3])) { $title = $_SERVER['argv'][3]; }
if(isset($_SERVER['argv'][4])) { $timespan = $_SERVER['argv'][4]; }


class PDF extends FPDF {
   public $my_title;

   public function Header() {
      $this->SetFont('Arial','B',15);
      $this->Cell(0, 0, $this->my_title, 0, 0, 'C');

      $this->Ln(20);
   }

   function Footer() {
      //Position at 1.5 cm from bottom
      $this->SetY(-15);

      $this->SetFont('Arial', 'I', 8);

      $this->Cell(0, 8, SITE_NAME . " - " . date("Y.m.d. H:i:s"), 0, 2, 'L');

      $this->Cell(0, 8, 'Page '.$this->PageNo(), 0, 0, 'R');
   }
}


$pdf = new PDF('L', 'mm', 'A4');
$pdf->my_title = $pdf_title;

$pdf->setTitle($pdf_title);

$pdf->AliasNbPages();
$pdf->AddPage();
$pdf->SetFont('Times','',12);

$tempfile = createTempName(DIR_REPORT, $timespan) . ".png";
$aa->lineChartHamSpam("", $timespan, $title, SIZE_X, SIZE_Y, $tempfile);
$pdf->Image($tempfile, 5, 20); unlink($tempfile);

$tempfile = createTempName(DIR_REPORT, $timespan) . ".png";
$aa->pieChartHamSpam("", $timespan, $title, $tempfile);
$pdf->Image($tempfile, 150, 20); unlink($tempfile);

$tempfile = createTempName(DIR_REPORT, $timespan) . ".png";
$aa->horizontalChartTopDomains("SPAM", $timespan, "Top SPAM sending domains", $tempfile);
$pdf->Image($tempfile, 5, 100); unlink($tempfile);

$tempfile = createTempName(DIR_REPORT, $timespan) . ".png";
$aa->horizontalChartTopDomains("HAM", $timespan, "Top HAM sending domains", $tempfile);
$pdf->Image($tempfile, 150, 100); unlink($tempfile);

$pdf->Output(DIR_REPORT . SITE_NAME . "-" . date("YmdHis") . "-" . $timespan . ".pdf");


?>
