DROP PROCEDURE IF EXISTS updateclapf1;

delimiter //

CREATE PROCEDURE updateclapf1()
BEGIN
  DECLARE _gid INT;
  DECLARE b INT;
  DECLARE cur1 CURSOR FOR SELECT uid FROM user;
  DECLARE CONTINUE HANDLER FOR NOT FOUND 
     SET b = 1;

  OPEN cur1;

  REPEAT
    FETCH cur1 INTO _gid;
    UPDATE user SET gid=_gid WHERE uid=_gid;
    UNTIL b = 1
  END REPEAT;

  CLOSE cur1;
END;
//

DELIMITER ;

CALL updateclapf1();

DROP PROCEDURE IF EXISTS updateclapf1;

