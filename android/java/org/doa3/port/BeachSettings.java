package org.doa3.port;

/** Preference domains shared with the original game activity. */
final class BeachSettings {
    private static final String[] ASPECTS={"source","4:3","16:9","stretch"};
    static String aspectAt(int index){return index>=0&&index<ASPECTS.length?ASPECTS[index]:ASPECTS[0];}
    static int aspectIndex(String aspect){
        for(int i=0;i<ASPECTS.length;i++)if(ASPECTS[i].equals(aspect))return i;
        return 0;
    }
    static boolean validAspect(String aspect){return aspectAt(aspectIndex(aspect)).equals(aspect);}
    static String[] aspectLabels(boolean english){
        return new String[]{english?"Original proportions":"Исходные пропорции","4:3","16:9",english?"Stretch to screen":"Растянуть на весь экран"};
    }
    static boolean valid(String aspect,String language,int resolution,int touch){
        return validAspect(aspect)
            &&("ru".equals(language)||"en".equals(language))
            &&resolution>=0&&resolution<=2&&touch>=0&&touch<=2;
    }
}
