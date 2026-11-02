#include "test.h"
#include "verbalize.h"

namespace verbalis::test {


REGISTER_TEST(numbers_hi_cardinal_test) {

  // Integers and decimals
  ASSERT_EQUALS(U"शून्य", verbalize("hi", U"0", only(&Options::numbers)));
  ASSERT_EQUALS(U"इक्कीस", verbalize("hi", U"21", only(&Options::numbers)));
  ASSERT_EQUALS(U"सत्तर", verbalize("hi", U"70", only(&Options::numbers)));
  ASSERT_EQUALS(U"इकहत्तर", verbalize("hi", U"71", only(&Options::numbers)));
  ASSERT_EQUALS(U"अस्सी", verbalize("hi", U"80", only(&Options::numbers)));
  ASSERT_EQUALS(U"निन्यानवे", verbalize("hi", U"99", only(&Options::numbers)));
  ASSERT_EQUALS(U"एक सौ", verbalize("hi", U"100", only(&Options::numbers)));
  ASSERT_EQUALS(U"एक सौ तेईस", verbalize("hi", U"123", only(&Options::numbers)));
  ASSERT_EQUALS(U"दो सौ", verbalize("hi", U"200", only(&Options::numbers)));
  ASSERT_EQUALS(U"एक हज़ार", verbalize("hi", U"1000", only(&Options::numbers)));
  ASSERT_EQUALS(U"दो हज़ार", verbalize("hi", U"2000", only(&Options::numbers)));
  ASSERT_EQUALS(U"पाँच हज़ार", verbalize("hi", U"5000", only(&Options::numbers)));
  ASSERT_EQUALS(U"एक लाख", verbalize("hi", U"100000", only(&Options::numbers)));
  ASSERT_EQUALS(U"दस लाख", verbalize("hi", U"1000000", only(&Options::numbers)));
  ASSERT_EQUALS(U"एक करोड़", verbalize("hi", U"10000000", only(&Options::numbers)));
  ASSERT_EQUALS(U"एक सौ तेईस दशमलव चार पाँच", verbalize("hi", U"123.45", only(&Options::numbers)));
  ASSERT_EQUALS(U"शून्य दशमलव शून्य पाँच", verbalize("hi", U"0.05", only(&Options::numbers)));

  // Fractions
  ASSERT_EQUALS(U"आधा", verbalize("hi", U"1/2", only(&Options::numbers)));
  ASSERT_EQUALS(U"एक तिहाई", verbalize("hi", U"1/3", only(&Options::numbers)));
  ASSERT_EQUALS(U"एक चौथाई", verbalize("hi", U"1/4", only(&Options::numbers)));
  ASSERT_EQUALS(U"तीन चौथाई", verbalize("hi", U"3/4", only(&Options::numbers)));
  ASSERT_EQUALS(U"तीन बटा दस", verbalize("hi", U"3/10", only(&Options::numbers)));

  // Currency
  ASSERT_EQUALS(U"एक डॉलर", verbalize("hi", U"1$", only(&Options::numbers)));
  ASSERT_EQUALS(U"पचास डॉलर", verbalize("hi", U"50$", only(&Options::numbers)));
  ASSERT_EQUALS(U"दो पाउंड", verbalize("hi", U"2£", only(&Options::numbers)));
  ASSERT_EQUALS(U"क़ीमत निन्यानवे यूरो है।", verbalize("hi", U"क़ीमत 99€ है।", only(&Options::numbers)));

  return true;
}

REGISTER_TEST(numbers_hi_ordinal_test) {

  // The suffix gives the form: masculine (वाँ), feminine (वीं) or plural (वें)
  ASSERT_EQUALS(U"पहला", verbalize("hi", U"1वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"पहली", verbalize("hi", U"1वीं", only(&Options::numbers)));
  ASSERT_EQUALS(U"पहले", verbalize("hi", U"1वें", only(&Options::numbers)));
  ASSERT_EQUALS(U"दूसरा", verbalize("hi", U"2वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"तीसरा", verbalize("hi", U"3वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"चौथा", verbalize("hi", U"4वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"पाँचवाँ", verbalize("hi", U"5वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"पाँचवीं", verbalize("hi", U"5वीं", only(&Options::numbers)));
  ASSERT_EQUALS(U"छठा", verbalize("hi", U"6वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"दसवाँ", verbalize("hi", U"10वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"बारहवाँ", verbalize("hi", U"12वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"इक्कीसवाँ", verbalize("hi", U"21वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"एक सौवाँ", verbalize("hi", U"100वाँ", only(&Options::numbers)));
  ASSERT_EQUALS(U"यह पाँचवीं बार है।", verbalize("hi", U"यह 5वीं बार है।", only(&Options::numbers)));

  return true;
}

REGISTER_TEST(numbers_hi_date_test) {

  ASSERT_EQUALS(U"सत्ताईस मार्च दो हज़ार छब्बीस", verbalize("hi", U"27.03.2026", only(&Options::numbers)));
  ASSERT_EQUALS(U"आज सत्ताईस मार्च दो हज़ार छब्बीस है।", verbalize("hi", U"आज 27-03-2026 है।", only(&Options::numbers)));
  ASSERT_EQUALS(U"आज सत्ताईस मार्च दो हज़ार छब्बीस है।", verbalize("hi", U"आज 2026-03-27 है।", only(&Options::numbers)));

  return true;
}

REGISTER_TEST(numbers_hi_context_test) {

  // Scales up to शंख; long numbers as numbers, but mobile numbers and codes after their word digit by digit
  ASSERT_EQUALS(U"इक्यावन अरब दो करोड़ तेईस लाख पैंतालीस हज़ार पाँच सौ सड़सठ", verbalize("hi", U"51022345567"));
  ASSERT_EQUALS(U"मेरा नंबर नौ आठ सात छह पाँच चार तीन दो एक शून्य है।", verbalize("hi", U"मेरा नंबर 9876543210 है।"));
  ASSERT_EQUALS(U"पिन कोड एक एक शून्य शून्य दो तीन", verbalize("hi", U"पिन कोड 110023"));
  ASSERT_EQUALS(U"लखनऊ, उत्तर प्रदेश दो दो छह शून्य शून्य एक", verbalize("hi", U"लखनऊ, उत्तर प्रदेश 226001"));
  ASSERT_EQUALS(U"मंगलवार को इक्यावन हज़ार तीन सौ चालीस.", verbalize("hi", U"मंगलवार को 51340."));

  // Years in hundreds where the words around mark them, and in dates
  ASSERT_EQUALS(U"उनका जन्म उन्नीस सौ सैंतालीस में हुआ।", verbalize("hi", U"उनका जन्म 1947 में हुआ।"));
  ASSERT_EQUALS(U"सन अठारह सौ सत्तावन की क्रांति", verbalize("hi", U"सन 1857 की क्रांति"));
  ASSERT_EQUALS(U"दो जुलाई उन्नीस सौ सत्तर", verbalize("hi", U"02-07-1970"));
  ASSERT_EQUALS(U"तीन हज़ार दो सौ से ज्यादा", verbalize("hi", U"3200 से ज्यादा"));
  ASSERT_EQUALS(U"उन्नीस सौ नब्बे ईसवी में", verbalize("hi", U"1990 ई. में"));
  ASSERT_EQUALS(U"दो सौ सत्तानवे से दो सौ बहत्तर ईसा पूर्व में", verbalize("hi", U"297-272 ई. पू. में"));
  ASSERT_EQUALS(U"छह मई", verbalize("hi", U"06-05"));
  ASSERT_EQUALS(U"दस से पंद्रह लोग", verbalize("hi", U"10-15 लोग"));

  // Times with seconds and with a dot, money with its smaller unit, units
  ASSERT_EQUALS(U"बारह बजकर दस मिनट दस सेकंड", verbalize("hi", U"12:10:10"));
  ASSERT_EQUALS(U"रात ग्यारह पचपन बजे", verbalize("hi", U"रात 11.55 बजे"));

  // Prime marks: three parts are a time; two are feet and inches by a word of height
  ASSERT_EQUALS(U"समय एक मिनट तेईस दशमलव चार पाँच सेकंड रहा।", verbalize("hi", U"समय 1'23\"45 रहा।"));
  ASSERT_EQUALS(U"उसकी लंबाई पाँच फीट नौ इंच है।", verbalize("hi", U"उसकी लंबाई 5'9\" है।"));
  ASSERT_EQUALS(U"इसकी कीमत नौ रुपये पचास पैसे है।", verbalize("hi", U"इसकी कीमत ₹9.50 है।"));
  ASSERT_EQUALS(U"एक रुपया पचास पैसे", verbalize("hi", U"₹1.50"));
  ASSERT_EQUALS(U"निन्यानवे सेंट", verbalize("hi", U"$0.99"));
  ASSERT_EQUALS(U"एक दशमलव पाँच मिलियन डॉलर", verbalize("hi", U"$1.5 मिलियन"));
  ASSERT_EQUALS(U"ढाई साल", verbalize("hi", U"2.5 yr"));
  ASSERT_EQUALS(U"दो सौ बाई दस के गद्दे", verbalize("hi", U"200x10 के गद्दे"));

  // Ordinals and Roman numerals
  ASSERT_EQUALS(U"सेवंथ क्रॉस", verbalize("hi", U"7th क्रॉस"));
  ASSERT_EQUALS(U"कक्षा बारह की परीक्षा", verbalize("hi", U"कक्षा XII की परीक्षा"));
  ASSERT_EQUALS(U"चंद्रयान-तीन", verbalize("hi", U"चंद्रयान-III"));
  ASSERT_EQUALS(U"बारहवीं कक्षा", verbalize("hi", U"XIIवीं कक्षा"));

  // Phones, codes, ranges, fractions
  ASSERT_EQUALS(U"प्लस नौ एक नौ आठ सात छह पाँच चार तीन दो एक शून्य", verbalize("hi", U"+91 9876543210"));
  ASSERT_EQUALS(U"शून्य तीन आठ छह दो तीन पाँच एक सात नौ एक", verbalize("hi", U"03862-351791"));
  ASSERT_EQUALS(U"टी बीस मैच", verbalize("hi", U"टी20 मैच"));
  ASSERT_EQUALS(U"ए बी सी एक सौ तेईस", verbalize("hi", U"ABC-123"));
  ASSERT_EQUALS(U"COVID उन्नीस", verbalize("hi", U"COVID-19"));
  ASSERT_EQUALS(U"दो क्यूब", verbalize("hi", U"2^3"));
  ASSERT_EQUALS(U"एक-आठ सौ-पाँच सौ पचपन", verbalize("hi", U"1-800-555"));
  ASSERT_EQUALS(U"चौदह बटा तीन, मथुरा रोड", verbalize("hi", U"14/3, मथुरा रोड"));
  ASSERT_EQUALS(U"सत्तानवे दशमलव शून्य", verbalize("hi", U"97.0"));

  // Titles, addresses, paths
  ASSERT_EQUALS(U"डॉक्टर अमित", verbalize("hi", U"डॉ. अमित"));
  ASSERT_EQUALS(U"रोक ले.", verbalize("hi", U"रोक ले."));
  ASSERT_EQUALS(U"एक नौ दो डॉट एक छह आठ डॉट एक डॉट एक", verbalize("hi", U"192.168.1.1"));
  ASSERT_EQUALS(U"C कोलन बैकवर्ड स्लैश Users बैकवर्ड स्लैश HP", verbalize("hi", U"C:\\Users\\HP"));
  ASSERT_EQUALS(U"report डॉट pdf", verbalize("hi", U"report.pdf"));
  ASSERT_EQUALS(U"(PCB/PCM) विषय", verbalize("hi", U"(PCB/PCM) विषय"));
  ASSERT_EQUALS(U"green हाइफ़न turner डॉट org", verbalize("hi", U"green-turner.org"));

  ASSERT_EQUALS(U"कोविड -उन्नीस के समय तापमान माइनस सात था।",
                verbalize("hi", U"कोविड -19 के समय तापमान -7 था।", only(&Options::numbers)));

  return true;
}

} // namespace verbalis::test
