
USE university;
SELECT * FROM students WHERE id > 0;

SELECT * FROM students WHERE id BETWEEN 1 AND 3;

SELECT * FROM students WHERE name LIKE "I.*";
SELECT * FROM students WHERE name LIKE ".*a.*";

SELECT COUNT(*) FROM students;
SELECT AVG(score) FROM students;

INSERT INTO students (id, name, score, status) VALUE (51, "Русский", 99, "Я");

UPDATE students SET score = 8888 WHERE id == 51;

SELECT * FROM students;

DELETE FROM students WHERE id == 51;

SELECT * FROM students;
