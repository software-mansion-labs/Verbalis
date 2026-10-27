# Hindi gold set: edits and unreachable cases

Edits to `tests/gold/hi.jsonl`, each with its reason. Verbalis's reading is added to a case's `outputs` only where a
native speaker accepts it as equally correct; a reading that is plainly wrong is corrected.

## Spelling variants

Hindi writes several numbers and words two standard ways: 91-99 with -नवे or -नबे (निन्यानवे, निन्यानबे), रुपये or
रुपए, चवालीस or चौवालीस, तिरेपन or तिरपन. The gold set uses the second form; Verbalis writes the first. Verbalis's
reading is added where it differs from a gold reading by these variants only (113 cases):

- `10099`: दस हज़ार निन्यानवे
- `७४४`: सात सौ चवालीस
- `191`: एक सौ इक्यानवे
- `१९९`: एक सौ निन्यानवे
- `१०९९`: एक हज़ार निन्यानवे
- `91`: इक्यानवे
- `९२`: बानवे
- `93`: तिरानवे
- `९४`: चौरानवे
- `95`: पंचानवे
- `९६`: छियानवे
- `97`: सत्तानवे
- `९८`: अट्ठानवे
- `99`: निन्यानवे
- `9876789`: अट्ठानवे लाख छिहत्तर हज़ार सात सौ नवासी
- `४,९९,९९,०००`: चार करोड़ निन्यानवे लाख निन्यानवे हज़ार
- `297-272 ई. पू.`: दो सौ सत्तानवे से दो सौ बहत्तर ईसा पूर्व.
- `1999 में`: उन्नीस सौ निन्यानवे में
- `1992 की`: उन्नीस सौ बानवे की
- `सन 1999`: सन उन्नीस सौ निन्यानवे
- `सन 1999 में`: सन उन्नीस सौ निन्यानवे में
- `99.99`: निन्यानवे दशमलव नौ नौ
- `९७.०`: सत्तानवे दशमलव शून्य
- `१९२.१६८`: एक सौ बानवे दशमलव एक छह आठ
- `192.168`: एक सौ बानवे दशमलव एक छह आठ
- `९९.९९`: निन्यानवे दशमलव नौ नौ
- `99/99`: निन्यानवे बटा निन्यानवे
- `97/0`: सत्तानवे बटा शून्य
- `१००००/९९९`: दस हज़ार बटा नौ सौ निन्यानवे
- `97 dg`: सत्तानवे डेसीग्राम
- `९७.७७ dg`: सत्तानवे दशमलव सात सात डेसीग्राम
- `93 lb`: तिरानवे पाउंड
- `९३.४ lb`: तिरानवे दशमलव चार पाउंड
- `99 oz`: निन्यानवे आउन्स
- `९९.५ oz`: निन्यानवे दशमलव पाँच आउन्स
- `₹111`: एक सौ ग्यारह रुपये
- `₹22`: बाईस रुपये
- `$696`: छह सौ छियानवे डॉलर
- `₹१४४`: एक सौ चवालीस रुपये
- `₹५३०`: पाँच सौ तीस रुपये
- `₹2132`: दो हज़ार एक सौ बत्तीस रुपये
- `₹१९८`: एक सौ अट्ठानवे रुपये
- `₹1115`: एक हज़ार एक सौ पंद्रह रुपये
- `₹३१८०`: तीन हज़ार एक सौ अस्सी रुपये
- `₹245`: दो सौ पैंतालीस रुपये
- `₹२१४८`: दो हज़ार एक सौ अड़तालीस रुपये
- `₹१५७४`: एक हज़ार पाँच सौ चौहत्तर रुपये
- `₹२७५`: दो सौ पचहत्तर रुपये
- `₺291`: दो सौ इक्यानवे लीरा
- `₹650`: छह सौ पचास रुपये
- `₹2190`: दो हज़ार एक सौ नब्बे रुपये
- `₹१०००`: एक हज़ार रुपये
- `₹५१३५`: पाँच हज़ार एक सौ पैंतीस रुपये
- `₹322`: तीन सौ बाईस रुपये
- `₩195`: एक सौ पंचानवे वॉन
- `₹1000`: एक हज़ार रुपये
- `₹405`: चार सौ पाँच रुपये
- `₹47`: सैंतालीस रुपये
- `₹220`: दो सौ बीस रुपये
- `₹५१`: इक्यावन रुपये
- `₹६३३`: छह सौ तैंतीस रुपये
- `₹320`: तीन सौ बीस रुपये
- `₹४६७`: चार सौ सड़सठ रुपये
- `₹811`: आठ सौ ग्यारह रुपये
- `₹१०४०`: एक हज़ार चालीस रुपये
- `₩५९९`: पाँच सौ निन्यानवे वॉन
- `₹550`: पाँच सौ पचास रुपये
- `₹६५६००`: पैंसठ हज़ार छह सौ रुपये
- `₹189`: एक सौ नवासी रुपये
- `₹५९१`: पाँच सौ इक्यानवे रुपये
- `₹1689`: एक हज़ार छह सौ नवासी रुपये
- `₹१०९`: एक सौ नौ रुपये
- `₹२०३`: दो सौ तीन रुपये
- `₹635`: छह सौ पैंतीस रुपये
- `₹४६९`: चार सौ उनहत्तर रुपये
- `₹55000000`: पाँच करोड़ पचास लाख रुपये
- `₹1254000`: बारह लाख चौवन हज़ार रुपये
- `₹३१५`: तीन सौ पंद्रह रुपये
- `₹2044`: दो हज़ार चवालीस रुपये
- `₹१००००`: दस हज़ार रुपये
- `₹545`: पाँच सौ पैंतालीस रुपये
- `₹१८४५`: एक हज़ार आठ सौ पैंतालीस रुपये
- `₹372`: तीन सौ बहत्तर रुपये
- `$९८`: अट्ठानवे डॉलर
- `₹123.57`: एक सौ तेईस रुपये सत्तावन पैसे
- `₹९९९.५०`: नौ सौ निन्यानवे रुपये पचास पैसे
- `$९.९९`: नौ डॉलर निन्यानवे सेंट
- `₹2.2000`: दो रुपये बीस पैसे
- `₹5.00`: पाँच रुपये
- `₹२.१२३`: दो दशमलव एक दो तीन रुपये
- `₹१.१२३४`: एक दशमलव एक दो तीन चार रुपये
- `₹3,24,50,000`: तीन करोड़ चौबीस लाख पचास हज़ार रुपये
- `₹32,450,000`: तीन करोड़ चौबीस लाख पचास हज़ार रुपये
- `₹5,50,00,000`: पाँच करोड़ पचास लाख रुपये
- `₹12,54,000`: बारह लाख चौवन हज़ार रुपये
- `₹1,00,000`: एक लाख रुपये
- `₹2,148`: दो हज़ार एक सौ अड़तालीस रुपये
- `₹99,999`: निन्यानवे हज़ार नौ सौ निन्यानवे रुपये
- `₹३,२४,५०,०००`: तीन करोड़ चौबीस लाख पचास हज़ार रुपये
- `₹३२,४५०,०००`: तीन करोड़ चौबीस लाख पचास हज़ार रुपये
- `₹२,१२,१५,०००`: दो करोड़ बारह लाख पंद्रह हज़ार रुपये
- `₹५,५०,००,०००`: पाँच करोड़ पचास लाख रुपये
- `₹१२,५४,०००`: बारह लाख चौवन हज़ार रुपये
- `₹५,५६,३२०`: पाँच लाख छप्पन हज़ार तीन सौ बीस रुपये
- `₹१,००,०००`: एक लाख रुपये
- `₹२,१४८`: दो हज़ार एक सौ अड़तालीस रुपये
- `₹९९,९९९`: निन्यानवे हज़ार नौ सौ निन्यानवे रुपये
- `91वीं`: इक्यानवेवीं
- `९९वां`: निन्यानवेवां
- `१०९१वें`: एक हज़ार इक्यानवेवें
- `LIII वा गणतंत्र दिन`: तिरेपनवा गणतंत्र दिन
- `99-1`: निन्यानवे-एक
- `1-800-999`: एक-आठ सौ-नौ सौ निन्यानवे

## Equally correct readings

- `१४७० एस वाशिंगटन स्ट्रीट`: added "एक हज़ार चार सौ सत्तर एस वाशिंगटन स्ट्रीट": a house number reads as a number as well as digit by digit.
- `579 ट्रॉय-शेंक्टाडी रोड`: added "पाँच सौ उनासी ट्रॉय-शेंक्टाडी रोड": the hyphen of a place name is not said.
- `७८३० - ई वेटरन्स पार्कवे, कोलंबस, जी ए ३१९०९`: added "सात हज़ार आठ सौ तीस - ई वेटरन्स पार्कवे, कोलंबस, जी ए तीन एक नौ शून्य नौ": a house number reads as a number; a spaced dash is a pause, not हाइफ़न.
- `66-4, पार्कहर्स्ट रोड`: added "छियासठ-चार, पार्कहर्स्ट रोड": the hyphen of a house number is a pause, not हाइफ़न.
- `८४०/१, १०० फीट रोड, मेट्रो पिलर ५६-५७, इंदिरानगर, बैंगलोर`: added "आठ सौ चालीस बटा एक, एक सौ फीट रोड, मेट्रो पिलर छप्पन से सत्तावन, इंदिरानगर, बैंगलोर": pillars 56-57 read as a range as well.
- `17-18, राजलक्ष्मी नगर, 7th क्रॉस स्ट्रीट, 100 फीट बाईपास रोड, वेलाचेरी, चेन्नई`: added "सत्रह से अठारह, राजलक्ष्मी नगर, सेवंथ क्रॉस स्ट्रीट, एक सौ फीट बाईपास रोड, वेलाचेरी, चेन्नई": plots 17-18 read as a range as well.
- `२५१३ ५३ एवेन्यू, मुंबई, महाराष्ट्र ४००००१`: added "दो हज़ार पाँच सौ तेरह तिरेपन एवेन्यू, मुंबई, महाराष्ट्र चार शून्य शून्य शून्य शून्य एक": a house number reads as a number as well as digit by digit.
- `john20@yahoo.com`: added "john बीस एट yahoo डॉट com": digits of a user name read as a number as well as digit by digit.
- `vivaan62@gmail.com`: added "vivaan बासठ एट gmail डॉट com": digits of a user name read as a number as well as digit by digit.
- `viaan15@kumaar.com`: added "viaan पंद्रह एट kumaar डॉट com": digits of a user name read as a number as well as digit by digit.
- `ltaa12@gmail.com`: added "ltaa बारह एट gmail डॉट com": digits of a user name read as a number as well as digit by digit.
- `kristen11@hotmail.com`: added "kristen ग्यारह एट hotmail डॉट com": digits of a user name read as a number as well as digit by digit.
- `https://google.com/`: added "google डॉट com फॉरवर्ड स्लैश": the scheme (https://) is not said, as in every language of Verbalis.
- `https://github.com/`: added "github डॉट com फॉरवर्ड स्लैश": the scheme (https://) is not said, as in every language of Verbalis.
- `https://wikipedia.org/`: added "wikipedia डॉट org फॉरवर्ड स्लैश": the scheme (https://) is not said, as in every language of Verbalis.
- `https://amazon.com/`: added "amazon डॉट com फॉरवर्ड स्लैश": the scheme (https://) is not said, as in every language of Verbalis.
- `https://www.ndtv.com`: added "w w w डॉट ndtv डॉट com": the scheme (https://) is not said, as in every language of Verbalis.
- `https://www.rbi.org.in/`: added "w w w डॉट rbi डॉट org डॉट in फॉरवर्ड स्लैश": the scheme (https://) is not said, as in every language of Verbalis.
- `https://www.amity.edu`: added "w w w डॉट amity डॉट edu": the scheme (https://) is not said, as in every language of Verbalis.
- `https://example.com/blog/`: added "example डॉट com फॉरवर्ड स्लैश blog फॉरवर्ड स्लैश": the scheme (https://) is not said, as in every language of Verbalis.
- `https://example.com/about.html`: added "example डॉट com फॉरवर्ड स्लैश about डॉट html": the scheme (https://) is not said, as in every language of Verbalis.
- `https://example.com/search.php`: added "example डॉट com फॉरवर्ड स्लैश search डॉट php": the scheme (https://) is not said, as in every language of Verbalis.
- `http://ati.edu`: added "ati डॉट edu": the scheme (https://) is not said, as in every language of Verbalis.
- `http://gcu.edu`: added "gcu डॉट edu": the scheme (https://) is not said, as in every language of Verbalis.
- `http://pima.edu`: added "pima डॉट edu": the scheme (https://) is not said, as in every language of Verbalis.
- `१/३`: added "एक तिहाई": एक तिहाई is the standard form of the bare तिहाई.
- `1/4`: added "एक चौथाई": एक चौथाई is the standard form of the bare चौथाई.
- `1000 yr`: added "एक हज़ार साल": साल and वर्ष both say year; other yr cases of the set use साल.
- `९९९९ yr`: added "नौ हज़ार नौ सौ निन्यानवे साल": साल and वर्ष both say year.
- `16.07 yr`: added "सोलह दशमलव शून्य सात साल": साल and वर्ष both say year.
- `1782वीं`: added "एक हज़ार सात सौ बयासीवीं": a count ordinal reads in हज़ार as well as in hundreds.
- `1981वीं`: added "एक हज़ार नौ सौ इक्यासीवीं": a count ordinal reads in हज़ार as well as in hundreds.
- `९८२६वीं`: added "नौ हज़ार आठ सौ छब्बीसवीं": नौ हज़ार आठ सौ is the standard reading of 9800; अट्ठानबे सौ is colloquial.
- `FY2024`: added "एफ वाई दो हज़ार चौबीस": a fiscal year reads as the year it names.
- `user@`: added "user एट": @ reads एट, as in every e-mail address.

## Corrected readings

- `मेरा पुराना नंबर था 9157114007`: corrected to "मेरा पुराना नंबर था नौ एक पाँच सात एक एक चार शून्य शून्य सात": the gold reading adds a zero the number doesn't have.
- `मेरे इस नंबर पे कॉल करो 1374-309988`: corrected to "मेरे इस नंबर पे कॉल करो एक तीन सात चार तीन शून्य नौ नौ आठ आठ": the gold reading adds a zero the number doesn't have.
- `मेरे इस नंबर पे कॉल करो 1374 309988`: corrected to "मेरे इस नंबर पे कॉल करो एक तीन सात चार तीन शून्य नौ नौ आठ आठ": the gold reading adds a zero the number doesn't have.

## Unreachable cases

- `२२ ३१/१७` (gold: बाईस और इकतीस बटा सत्रह): a mixed number never has an improper fraction, so the bare fragment
  doesn't say that 22 and 31/17 belong together; reading every whole number before an improper fraction as one mixed
  number would join unrelated numbers in running text, such as a count before a score.
- `Al₂(SO₄)₃` (gold: ए एल दो ओपन ब्रेकेट एस ओ चार क्लोज़ ब्रेकेट तीन): the gold reading says the parentheses as
  words, which would misread every parenthesis of running text, and spells the element symbol Al, which a rule can't
  tell from a short word.
