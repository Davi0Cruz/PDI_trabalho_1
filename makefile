run:
	gcc src/main.c -ltiff -o app.out
	./app.out clock_original.tif 300
	mv output.tif clock_a.tif
	./app.out clock_a.tif 1250
	mv output.tif clock_b.tif
	
	./app.out moon_original.tiff 10
	mv output.tif moon_a.tif
	./app.out moon_a.tif 100
	mv output.tif moon_b.tif
	
	./app.out city_original.tiff 10
	mv output.tif city_a.tif
	./app.out city_a.tif 100
	mv output.tif city_b.tif
