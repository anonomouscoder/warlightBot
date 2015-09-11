variable1=1
variable2=1000
variable3=0
variable4=500
variable5=1000

bestvar1=1
bestvar2=1000
bestvar3=0
bestvar4=500
bestvar5=1000

varianceFactor1=1
varianceFactor2=100
varianceFactor3=100
varianceFactor4=100
varianceFactor5=100

resultRegex="a: (\d+) \d+%"

maxNumberOfGeneticIterations=100 #idk
runAmount=50 # per map
bestResult=0
thisResult=0

for i in `seq 1 $maxNumberOfGeneticIterations`;
do
	printf "$variable1\n" >genetics.txt
	printf "$variable2\n" >>genetics.txt
	printf "$variable3\n" >>genetics.txt
	printf "$variable4\n" >>genetics.txt
	printf "$variable5\n" >>genetics.txt

	result=$(bash ../visualizer/runx.sh $runAmount)
	
	[[ $result =~ $resultRegex ]]
	thisResult="${BASH_REMATCH[1]}"
	
	if [ "$thisResult" -gt "$bestResult" ]; then
		bestvar1=$variable1
		bestvar2=$variable2
		bestvar3=$variable3
		bestvar4=$variable4
		bestvar5=$variable5
	fi
	
	#randomize variables
	rand=$RANDOM
	range=$[$varianceFactor1*2]
	let "rand %= $range"
	variable1=$[$variable1+$rand-$varianceFactor1]

	rand=$RANDOM
	range=$[$varianceFactor2*2]
	let "rand %= $range"
	variable2=$[$variable2+$rand-$varianceFactor2]

	rand=$RANDOM
	range=$[$varianceFactor3*2]
	let "rand %= $range"
	variable3=$[$variable3+$rand-$varianceFactor3]

	rand=$RANDOM
	range=$[$varianceFactor4*2]
	let "rand %= $range"
	variable4=$[$variable4+$rand-$varianceFactor4]

	rand=$RANDOM
	range=$[$varianceFactor5*2]
	let "rand %= $range"
	variable5=$[$variable5+$rand-$varianceFactor5]
done
