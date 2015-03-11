var nearestMealIndex;
var sendItemIndex = 0;
var nextMenu;
var metadata;
var doneMetadata = false;

function HTTPGET(url) {
	var req = new XMLHttpRequest();
	req.open('GET', url, true);
	req.onload = function(e) {
		if (req.readyState == 4 && req.status == 200) {
			if(req.status == 200) {
				parseAndSendMenu(req.responseText);
			} else {
				console.log('Error: ' + req.statusText);
				nextMenu = ['No Internet'];
				doneMetadata = false;
				metadata = {"MSGTYPE_KEY" : 0, "MESSAGE_KEY" : "...", "SIZE_KEY" : nextMenu.length};
				Pebble.sendAppMessage(metadata, handlePebbleACKMetadata, handlePebbleNACKMetadata);
			}
		}
	};
	req.send(null);
}

function getMenu() {
	HTTPGET("http://navy.herokuapp.com/menu");
}

function parseAndSendMenu(response) {
    var json = JSON.parse(response);

	nextMenu = createMenuCode(getNextThreeMeals(getNextTwoDayMenus(json)));

	console.log(nextMenu);
	console.log("size" + nextMenu.length);

	doneMetadata = false;
	metadata = {"MSGTYPE_KEY" : 0, "MESSAGE_KEY" : mealIndexToTitle(nearestMealIndex), "SIZE_KEY" : nextMenu.length};
	Pebble.sendAppMessage(metadata, handlePebbleACKMetadata, handlePebbleNACKMetadata);
}

function handlePebbleACKMetadata(e) {
	if (doneMetadata === false)
		sendMenuItem(sendItemIndex);
	return;
}

function handlePebbleNACKMetadata(e) {
	Pebble.sendAppMessage(metadata, handlePebbleACKMetadata, handlePebbleNACKMetadata);
}

function handlePebbleACKFoodItem(e) {
	sendItemIndex++;
	if (sendItemIndex == nextMenu.length) {
		sendItemIndex = 0;
		doneMetadata = true;
		//send done
		metadata = {"MSGTYPE_KEY" : 2, "MESSAGE_KEY" : mealIndexToTitle(nearestMealIndex), "SIZE_KEY" : nextMenu.length};
		Pebble.sendAppMessage(metadata, handlePebbleACKMetadata, handlePebbleNACKMetadata);
	} else {
		sendMenuItem(sendItemIndex);
	}
}

function handlePebbleNACKFoodItem(e) {
	sendMenuItem(sendItemIndex);
}

function sendMenuItem(i) {
	/*
	if (i < nextMenu.length) {
		var dict = {"MSGTYPE_KEY" : 1, "MESSAGE_KEY" : nextMenu[i], "SIZE_KEY" : nextMenu[i].length};
		console.log("send" + nextMenu[i]);
		Pebble.sendAppMessage(dict, handlePebbleACKFoodItem, handlePebbleNACKFoodItem);
	}
	*/
	var dict = {"MSGTYPE_KEY" : 1, "MESSAGE_KEY" : nextMenu[i], "SIZE_KEY" : nextMenu[i].length};
	console.log("send " + nextMenu[i]);
	Pebble.sendAppMessage(dict, handlePebbleACKFoodItem, handlePebbleNACKFoodItem);
}

Pebble.addEventListener("ready", function(e) {
    getMenu();
});

Pebble.addEventListener("appmessage", function(e) {
	if (sendItemIndex == 0)
    	getMenu();
} );



function createMenuCode(ntm) {
    //mealIndexToTitle(nearestMealIndex);

	var parsedMenu = [];

    for (var i = 0; i < ntm[0].length; i++) {
        if (ntm[0][i].title !== '') {
            parsedMenu.push(ntm[0][i].title);
        }
    }

	return parsedMenu;
}

function mealIndexToTitle(index) {
    switch (index) {
        case 0:
            return 'Breakfast';
        case 1:
            return 'Lunch';
        case 2:
            return 'Dinner';
        default:
            return 'Unknown';
    }
}

function getNextThreeMeals(twoDays) {
    var mealAbbrArr = ['B', 'L', 'D'];

    var dateObj = new Date();

    var hour = dateObj.getHours();
    var minute = dateObj.getMinutes();

    var dayIndex = 0;
    var mealIndex = 0;

    if (hour > 7 || hour == 7 && minute > 30) mealIndex++;

    if (hour > 13 || hour == 13 && minute > 30) mealIndex++;

    if (hour > 19 || hour == 19 && minute > 30) {
        dayIndex++;
        mealIndex = 0;
    }



    nearestMealIndex = mealIndex;

    var nextThreeMeals = [];
    //console.log(twoDays);
    for (var i = 0; i < 3; i++) {
        //console.log(dayIndex + '|' + mealIndex);
        //console.log(twoDays[dayIndex][mealAbbrArr[mealIndex]]);
        nextThreeMeals.push(twoDays[dayIndex][mealAbbrArr[mealIndex]]);
        if (mealIndex == 2) {
            dayIndex++;
            mealIndex = 0;
        } else {
            mealIndex++;
        }
    }

    return nextThreeMeals;
}

function getNextTwoDayMenus(data) {
    var dateArray = ['U', 'M', 'T', 'W', 'R', 'F', 'S'];

    var weekMenu = data;
    var dateObj = new Date();
    var weekdayIndex = dateObj.getDay();
    var dateAbbr = dateArray[weekdayIndex];

    var twoDayMenus = [];

    for (var i = 0; i < 2; i++) {
        dateAbbr = dateArray[weekdayIndex];
        //console.log(dateAbbr);
        twoDayMenus.push(weekMenu[dateAbbr]);


        if (weekdayIndex == 6) {
            weekdayIndex = 0;
        } else {
            weekdayIndex++;
        }
    }
    return twoDayMenus;
}
