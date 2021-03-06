<?php declare(strict_types=1);
/**
 * This file contains the NDB_Menu_Filter_media class
 *
 * PHP Version 7
 *
 * @category Module
 * @package  Media
 * @author   Alex Ilea <ailea.mcin@gmail.com>
 * @license  http://www.gnu.org/licenses/gpl-3.0.txt GPLv3
 * @link     https://www.github.com/aces/CCNA/
 */
namespace LORIS\media;

/**
 * Main class for media module corresponding to /media/ URL
 * Child or Clinical section of the LorisMenu.
 *
 * Displays a list of media files and control panel to search them and upload
 * new ones.
 *
 * PHP Version 7
 *
 * @category Module
 * @package  Media
 * @author   Alex Ilea <ailea.mcin@gmail.com>
 * @license  http://www.gnu.org/licenses/gpl-3.0.txt GPLv3
 * @link     https://www.github.com/aces/CCNA/
 */
class Media extends \NDB_Menu_Filter
{
    public $AjaxModule   = true;
    public $skipTemplate = true;

    /**
     * Check user permissions
     *
     * @param \User $user The user whose access is being checked
     *
     * @return bool
     */
    function _hasAccess(\User $user) : bool
    {
        return $user->hasAnyPermission(
            array(
             'media_read',
             'media_write',
            )
        );

    }

    /**
     * Create a form to filter media by various criteria
     *
     * @return array
     */
    function getOptions() : array
    {
        $user = \User::singleton();
        $db   = \Database::singleton();

        $siteList  = array();
        $visitList = \Utility::getVisitList();

        // allow to view all sites data through filter
        if ($user->hasPermission('access_all_profiles')) {
            $siteList = \Utility::getSiteList(false);
        } else {
            // allow only to view own site data
            $siteIDs = $user->getData('CenterIDs');
            foreach ($siteIDs as $val) {
                $site = \Site::singleton($val);
                if ($site->isStudySite()) {
                    $siteList[$val] = $site->getCenterName();
                }
            }
        }

        $instrumentList   = array();
        $instrumentsQuery = $db->pselect(
            "SELECT Test_name, Full_name FROM test_names ORDER BY Test_name",
            []
        );
        foreach ($instrumentsQuery as $instrument) {
            $instrumentList[$instrument['Full_name']] = $instrument['Full_name'];
        }

        $fileTypeList  = array();
        $fileTypeQuery = $db->pselect("SELECT file_type FROM media", []);
        foreach ($fileTypeQuery as $filetype) {
            $fileTypeList[$filetype['file_type']] = $filetype['file_type'];
        }

        //Language selector
        $languages    = \Utility::getLanguageList();
        $languageList = array();
        foreach ($languages as $language) {
            $languageList[$language] = $language;
        }

        $hiddenOptions = array();
        if (\User::singleton()->hasPermission('superuser')) {
            $hiddenOptions = array(
                              0 => 'Visible only',
                              1 => 'Hidden only',
                             );
        }

        return array(
                'fileTypes'     => $fileTypeList,
                'visits'        => $visitList,
                'sites'         => $siteList,
                'instruments'   => $instrumentList,
                'languages'     => $languageList,
                'hidden'        => $hiddenOptions,
                'maxUploadSize' => \Utility::getMaxUploadSize(),
               );
    }

    /**
     * Gets the data source for this menu filter.
     *
     * @return \LORIS\Data\Provisioner
     */
    function getDataProvisioner() : \LORIS\Data\Provisioner
    {
        $provisioner = new MediaFileProvisioner();

        $user = \User::singleton();

        if ($user->hasPermission("access_all_profiles") == false) {
            $provisioner = $provisioner->filter(
                new \LORIS\Data\Filters\UserSiteMatch()
            );
        }
        if (!$user->hasPermission("superuser")) {
            $provisioner = $provisioner->filter(
                new HideFileFilter()
            );
        }
        return $provisioner;
    }

    /**
     * Converts the results of this menu filter to a JSON format to be retrieved
     * with ?format=json
     *
     * @return string a json encoded string of the headers and data from this table
     */
    function toJSON() : string
    {
        $table = (new \LORIS\Data\Table())
            ->withDataFrom($this->getDataProvisioner());
        $arr   = array_map(
            function ($row) {
                return array_values($row);
            },
            json_decode($table->toJSON(\User::singleton()), true)
        );
        return json_encode(
            array(
             'data'         => $arr,
             'fieldOptions' => $this->getOptions(),
            )
        );
    }

    /**
     * Include additional CSS files:
     *  1. media.css
     *
     * @return array of javascript to be inserted
     */
    function getCSSDependencies() : array
    {
        $factory = \NDB_Factory::singleton();
        $baseURL = $factory->settings()->getBaseURL();
        $deps    = parent::getCSSDependencies();
        return array_merge(
            $deps,
            [$baseURL . "/media/css/media.css"]
        );
    }

    /**
     * Include additional JS files
     *
     * @return array of javascript to be inserted
     */
    function getJSDependencies() : array
    {
        $factory = \NDB_Factory::singleton();
        $baseURL = $factory->settings()->getBaseURL();
        $deps    = parent::getJSDependencies();
        return array_merge(
            $deps,
            array(
             $baseURL . "/media/js/mediaIndex.js",
            )
        );
    }
}

